#include "stat_dispatch.h"

using namespace std;

namespace actuary {

    void start_stat_zmq_server(ActuaryConfig &config, GlobalContext &context) {

        if (config.dt_group_main_node) {
            thread zmq_thread(stat_zmq_server_listen, ref(config), ref(context));
            zmq_thread.detach();
            info_log("start order stat zmq server listen");
        }
    }

    void start_delay_load_orders_to_zmq(ActuaryConfig &config, GlobalContext &context) {
        if (!config.dt_group_main_node) {
            info_log("only dt main node enable load recent order to zmq");
            return;
        }
        if (!config.enable_load_order_to_zmq) {
            warn_log("disable load recent order to zmq at beginning.");
            return;
        }
        thread zmq_thread(load_recent_orders_to_zmq, ref(config), ref(context));
        zmq_thread.detach();
        info_log("start delay task of load order stat to zmq");
    }

    void load_recent_orders_to_zmq(ActuaryConfig &config, GlobalContext &context) {

        // TODO wait subscriber establish connection
        std::this_thread::sleep_for(std::chrono::seconds(30));
        // load orders which in one hour
        uint64_t system_timestamp_start = binance::get_current_epoch() - 3600;
        string sql = fmt::format("select account_flag, symbol, order_side, client_order_id, average_price, filled_size, system_timestamp, commission_rate from tb_bnum_order where system_timestamp >= {} and account_flag = '{}'", system_timestamp_start, config.account_flag);
        
        info_log("select order for stat sql: {}", sql);
        
        MYSQL* my_conn = context.get_mysql_source()->getConnection();
        if (my_conn != nullptr) {
            try {
                if (mysql_query(my_conn, sql.c_str()) != 0) {
                    int my_no = mysql_errno(my_conn);
                    string my_err = mysql_error(my_conn);
                    err_log("fail to query order: {} {}", my_no, my_err);
                }
                MYSQL_RES *result = mysql_store_result(my_conn);
                if (result == nullptr) {
                    if (mysql_errno(my_conn)) {
                        int my_no = mysql_errno(my_conn);
                        string my_err = mysql_error(my_conn);
                        err_log("fail to get result from query order: {} {}", my_no, my_err);
                    }
                } else {
                    MYSQL_ROW row;
                    while ((row = mysql_fetch_row(result)) != NULL) {
                        StatOrderLite lite;
                        unsigned int fields_num = mysql_num_fields(result);
                        lite.accountFlag = row[0];
                        lite.symbol = row[1];
                        lite.orderSide = row[2]; 
                        lite.clientOrderId = row[3];
                        lite.averagePrice = row[4];
                        lite.filledSize = row[5];
                        lite.systemTimestamp = std::stoull(row[6]);
                        lite.commissionRate = row[7];

                        bool enqueue_result = context.get_stat_order_channel()->try_enqueue(lite);
                        if (!enqueue_result) {
                            warn_log("can not enqueue stat order {} {}", lite.symbol, lite.clientOrderId);
                        }
                    }
                    mysql_free_result(result);
                }
            } catch (exception &exp) {
                err_log("exception occur while query order: {}", string(exp.what()));
            }
            context.get_mysql_source()->releaseConnection(my_conn);
        } else {
            warn_log("no mysql connection created for query order");
        }
    }

    void stat_zmq_server_listen(ActuaryConfig &config, GlobalContext &context) {

        ZMQClient zmq_client(ZMQ_PUB);
        shared_ptr<moodycamel::ConcurrentQueue<StatOrderLite>> channel = context.get_stat_order_channel();

        while (true) {
            try {
                zmq_client.PublisherBind(config.zmq_ipc_order_stat);
                while (true) {
                    StatOrderLite orderLite;
                    while (!channel->try_dequeue(orderLite)) {
                        // Retry if the queue is empty
                        std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    }

                    Json::Value message;
                    message["accountFlag"] = orderLite.accountFlag;
                    message["symbol"] = orderLite.symbol;
                    message["orderSide"] = orderLite.orderSide;
                    message["clientOrderId"] = orderLite.clientOrderId;
                    message["averagePrice"] = orderLite.averagePrice;
                    message["filledSize"] = orderLite.filledSize;
                    message["systemTimestamp"] = orderLite.systemTimestamp;
                    message["commissionRate"] = orderLite.commissionRate;
                    message["message_timestamp_mills"] = binance::get_current_ms_epoch();

                    Json::StreamWriterBuilder writer;
                    std::string jsonString = Json::writeString(writer, message);
                    int jsonLength = jsonString.size();

                    zmq_client.Send(jsonString);

                    strHelper::replaceString(jsonString, "\n", "");
                    info_log("send order stat to zmq: {}", jsonString);
                }
            } catch (std::exception &exp) {
                err_log("fail to start order stat zmq publisher {}", string(exp.what()));
            }

            err_log("restart order stat zmq publisher after 5 seconds");

            // wait for a while after exception
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
    }

    void start_delay_save_exchange_info(ActuaryConfig &config, GlobalContext &context) {

        thread delay_thread(delay_save_exchange_info, ref(config), ref(context));
        delay_thread.detach();
        info_log("start delay task for save exchange info");
    }

    void delay_save_exchange_info(ActuaryConfig &config, GlobalContext &context) {

        this_thread::sleep_for(chrono::seconds(60));
        
        int saved_number = 0;
        MYSQL* my_conn = context.get_mysql_source()->getConnection();
        if (my_conn != nullptr) {
            for (string baseAsset : config.node_base_assets) {
                string inst_id = baseAsset + config.follower_quote_asset;
                auto exchange = context.get_exchange_info(inst_id);
                if (exchange == nullopt) {
                    warn_log("not found exchange info for {}", inst_id);
                    continue;
                }

                double ticker_price = 0;
                auto ticker_shm_index = context.get_shm_follower_ticker_mapping().find(inst_id);
                if (ticker_shm_index == context.get_shm_follower_ticker_mapping().end()) {
                    warn_log("not found ticker shm index for {}", inst_id);
                } else {
                    std::shared_ptr<shm_mng::TickerInfoShm> follower_ticker = shm_mng::ticker_shm_reader_get(context.get_shm_store_info().follower_start, ticker_shm_index->second);
                    if (follower_ticker == nullptr) {
                        warn_log("not found ticker in shm for {}", inst_id);
                    } else {
                        // use bid as ticker price for judgement
                        ticker_price = follower_ticker->bid_price;
                    }
                }

                string sql = fmt::format("insert into tb_bnum_exchange_info "
                    " (account_flag, symbol, ticker_price, ticker_size, step_size, price_precision, quantity_precision, create_time) values "
                    " ('{}', '{}', {}, {}, {}, {}, {}, now()) on duplicate key update "
                    " ticker_price={}, ticker_size={}, step_size={}, price_precision={}, quantity_precision={}, enabled='Y'",
                    config.account_flag, exchange.value().symbol, ticker_price, exchange.value().tickSize, exchange.value().stepSize, exchange.value().pricePrecision, exchange.value().quantityPrecision,
                    ticker_price, exchange.value().tickSize, exchange.value().stepSize, exchange.value().pricePrecision, exchange.value().quantityPrecision
                );
                info_log("save exchange info sql: {}", sql);
                try {
                    if (mysql_query(my_conn, sql.c_str()) != 0) {
                        int my_no = mysql_errno(my_conn);
                        string my_err = mysql_error(my_conn);
                        err_log("fail to save exchange info: {} {} {}", exchange.value().symbol ,my_no, my_err);
                    } else {
                        saved_number++;
                    }
                } catch (exception &exp) {
                    err_log("exception occur while save exchange info: {}", string(exp.what()));
                }
            }
            context.get_mysql_source()->releaseConnection(my_conn);
        } else {
            warn_log("no mysql connection created");
        }

        info_log("finish save exchange info. saved number is {}", saved_number);
    }
}