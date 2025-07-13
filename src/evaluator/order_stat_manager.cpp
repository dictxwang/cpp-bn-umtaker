#include "order_stat_manager.h"

using namespace std;

namespace evaluator {

    void start_order_stat_manager(EvaluatorConfig& config, GlobalContext& context) {

        load_recently_order_stat_result(config, context);
        info_log("finish load recently order stat result");

        std::thread polling_thread(polling_calculate_order_pnl, std::ref(config), std::ref(context));
        polling_thread.detach();
    }

    void load_recently_order_stat_result(EvaluatorConfig& config, GlobalContext& context) {

        vector<string> account_flag_list;
        for (string account : config.accounts_need_statistics) {
            account_flag_list.push_back("'" + account + "'");
        }
        string account_flag_condition = strHelper::joinStrings(account_flag_list, ",");
        uint64_t system_timestamp_condition = binance::get_current_epoch() - 3610;

        string sql = fmt::format("select account_flag, symbol, order_side, client_order_id, average_price, filled_size, system_timestamp, "
            " price_after_1s, price_after_5s, price_after_10s, price_after_20s, price_after_30s, price_after_40s, price_after_50s, "
            " price_after_60s, price_after_120s, price_after_180s, price_after_15min, price_after_30min, price_after_1hour, "
            " pnl_1s_percentage, pnl_5s_percentage, pnl_10s_percentage, pnl_20s_percentage, pnl_30s_percentage, pnl_40s_percentage, pnl_50s_percentage, "
            " pnl_60s_percentage, pnl_120s_percentage, pnl_180s_percentage, pnl_15min_percentage, pnl_30min_percentage, pnl_1hour_percentage, "
            " commission_rate from tb_bnum_order_pnl where system_timestamp >= {} and account_flag in ({}) order by system_timestamp",
        system_timestamp_condition, account_flag_condition);

        info_log("sql for select recent order stat: {}", sql);

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
                        StatOrderFullInfo info;
                        unsigned int fields_num = mysql_num_fields(result);
                        info.account_flag = row[0];
                        info.symbol = row[1];
                        info.order_side = row[2]; 
                        info.client_order_id = row[3];
                        info.average_price = std::stod(row[4]);
                        info.filled_size = std::stod(row[5]);
                        info.system_timestamp = std::stoull(row[6]);

                        info.price_after_1s = std::stod(row[7]);
                        info.price_after_5s = std::stod(row[8]);
                        info.price_after_10s = std::stod(row[9]);
                        info.price_after_20s = std::stod(row[10]);
                        info.price_after_30s = std::stod(row[11]);
                        info.price_after_40s = std::stod(row[12]);
                        info.price_after_50s = std::stod(row[13]);
                        info.price_after_60s = std::stod(row[14]);
                        info.price_after_120s = std::stod(row[15]);
                        info.price_after_180s = std::stod(row[16]);
                        info.price_after_15min = std::stod(row[17]);
                        info.price_after_30min = std::stod(row[18]);
                        info.price_after_1hour = std::stod(row[19]);
                        
                        info.pnl_1s_percentage = std::stod(row[20]);
                        info.pnl_5s_percentage = std::stod(row[21]);
                        info.pnl_10s_percentage = std::stod(row[22]);
                        info.pnl_20s_percentage = std::stod(row[23]);
                        info.pnl_30s_percentage = std::stod(row[24]);
                        info.pnl_40s_percentage = std::stod(row[25]);
                        info.pnl_50s_percentage = std::stod(row[26]);
                        info.pnl_60s_percentage = std::stod(row[27]);
                        info.pnl_120s_percentage = std::stod(row[28]);
                        info.pnl_180s_percentage = std::stod(row[29]);
                        info.pnl_15min_percentage = std::stod(row[30]);
                        info.pnl_30min_percentage = std::stod(row[31]);
                        info.pnl_1hour_percentage = std::stod(row[32]);
                        info.commission_rate = std::stod(row[33]);

                        bool updated = context.get_stat_order_composite().update_with_full_info(info);
                        info_log("load and update order full info: {} {} {} updated={}", info.account_flag, info.symbol, info.client_order_id, updated);
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
    
    void polling_calculate_order_pnl(EvaluatorConfig& config, GlobalContext& context) {

        while (true) {
            this_thread::sleep_for(chrono::milliseconds(200));

            uint64_t now = binance::get_current_epoch();
            vector<StatOrderFullInfo> info_list = context.get_stat_order_composite().get_all_stat_orders();

            for (StatOrderFullInfo& info : info_list) {
                info.property_changed = false;
                if (now <= info.system_timestamp) {
                    continue;
                }
                auto ticker = context.get_futures_ticker_composite().get_ticker(info.symbol);
                if (ticker == nullopt) {
                    warn_log("ticker not found for {}", info.symbol);
                    continue;
                }
                double current_price;
                int pnl_multiple;
                if (info.order_side == binance::ORDER_SIDE_BUY) {
                    current_price = ticker.value().ask;
                    pnl_multiple = 1;
                } else {
                    current_price = ticker.value().bid;
                    pnl_multiple = -1;
                }

                double pnl_percentage = ((current_price - info.average_price) / info.average_price) * pnl_multiple;
                pnl_percentage = decimal_process(pnl_percentage - info.commission_rate, 5);
                long time_range = now - info.system_timestamp;

                if (time_range >= 1 && info.price_after_1s == 0) {
                    info.price_after_1s = current_price;
                    info.pnl_1s_percentage = pnl_percentage;
                    info.property_changed = true;
                }
                if (time_range >= 5 && info.price_after_5s == 0) {
                    info.price_after_5s = current_price;
                    info.pnl_5s_percentage = pnl_percentage;
                    info.property_changed = true;
                }
                if (time_range >= 10 && info.price_after_10s == 0) {
                    info.price_after_10s = current_price;
                    info.pnl_10s_percentage = pnl_percentage;
                    info.property_changed = true;
                }
                if (time_range >= 20 && info.price_after_20s == 0) {
                    info.price_after_20s = current_price;
                    info.pnl_20s_percentage = pnl_percentage;
                    info.property_changed = true;
                }
                if (time_range >= 30 && info.price_after_30s == 0) {
                    info.price_after_30s = current_price;
                    info.pnl_30s_percentage = pnl_percentage;
                    info.property_changed = true;
                }
                if (time_range >= 40 && info.price_after_40s == 0) {
                    info.price_after_40s = current_price;
                    info.pnl_40s_percentage = pnl_percentage;
                    info.property_changed = true;
                }
                if (time_range >= 50 && info.price_after_50s == 0) {
                    info.price_after_50s = current_price;
                    info.pnl_50s_percentage = pnl_percentage;
                    info.property_changed = true;
                }
                if (time_range >= 60 && info.price_after_60s == 0) {
                    info.price_after_60s = current_price;
                    info.pnl_60s_percentage = pnl_percentage;
                    info.property_changed = true;
                }
                if (time_range >= 120 && info.price_after_120s == 0) {
                    info.price_after_120s = current_price;
                    info.pnl_120s_percentage = pnl_percentage;
                    info.property_changed = true;
                }
                if (time_range >= 180 && info.price_after_180s == 0) {
                    info.price_after_180s = current_price;
                    info.pnl_180s_percentage = pnl_percentage;
                    info.property_changed = true;
                }
                if (time_range >= 900 && info.price_after_15min == 0) {
                    info.price_after_15min = current_price;
                    info.pnl_15min_percentage = pnl_percentage;
                    info.property_changed = true;
                }
                if (time_range >= 1800 && info.price_after_30min == 0) {
                    info.price_after_30min = current_price;
                    info.pnl_30min_percentage = pnl_percentage;
                    info.property_changed = true;
                }
                if (time_range >= 3600 && info.price_after_1hour == 0) {
                    info.price_after_1hour = current_price;
                    info.pnl_1hour_percentage = pnl_percentage;
                    info.property_changed = true;
                }

                if (info.property_changed) {
                    bool updated = context.get_stat_order_composite().update_with_full_info(info);
                    if (!updated) {
                        warn_log("not update order full info for {}", info.client_order_id);
                    }
                }
            }

            // update data in db
            MYSQL* my_conn = context.get_mysql_source()->getConnection();
            for (StatOrderFullInfo& info : info_list) {
                if (now <= info.system_timestamp) {
                    continue;
                }
                if (!info.property_changed) {
                    continue;
                }

                string has_trading_volume = "N";
                if (info.filled_size > 0) {
                    has_trading_volume = "Y";
                }

                string sql = fmt::format("insert into tb_bnum_order_pnl ("
                    " account_flag, symbol, order_side, client_order_id, average_price, filled_size, system_timestamp, create_time, "
                    " price_after_1s, price_after_5s, price_after_10s, price_after_20s, price_after_30s, price_after_40s, price_after_50s, "
                    " price_after_60s, price_after_120s, price_after_180s, price_after_15min, price_after_30min, price_after_1hour, "
                    " pnl_1s_percentage, pnl_5s_percentage, pnl_10s_percentage, pnl_20s_percentage, pnl_30s_percentage, pnl_40s_percentage, pnl_50s_percentage, "
                    " pnl_60s_percentage, pnl_120s_percentage, pnl_180s_percentage, pnl_15min_percentage, pnl_30min_percentage, pnl_1hour_percentage, "
                    " commission_rate, has_trading_volume "
                    ") values ( "
                    " '{}', '{}', '{}', '{}', {}, {}, {}, now(), "
                    " {}, {}, {}, {}, {}, {}, {}, "
                    " {}, {}, {}, {}, {}, {}, "
                    " {}, {}, {}, {}, {}, {}, {}, "
                    " {}, {}, {}, {}, {}, {}, "
                    " {}, '{}' "
                    " ) on duplicate key update "
                    " filled_size= {}, has_trading_volume='{}', "
                    " price_after_1s={}, price_after_5s={}, price_after_10s={}, price_after_20s={}, price_after_30s={}, price_after_40s={}, price_after_50s={}, "
                    " price_after_60s={}, price_after_120s={}, price_after_180s={}, price_after_15min={}, price_after_30min={}, price_after_1hour={}, "
                    " pnl_1s_percentage={}, pnl_5s_percentage={}, pnl_10s_percentage={}, pnl_20s_percentage={}, pnl_30s_percentage={}, pnl_40s_percentage={}, pnl_50s_percentage={}, "
                    " pnl_60s_percentage={}, pnl_120s_percentage={}, pnl_180s_percentage={}, pnl_15min_percentage={}, pnl_30min_percentage={}, pnl_1hour_percentage={}",
                    info.account_flag, info.symbol, info.order_side, info.client_order_id, info.average_price, info.filled_size, info.system_timestamp,
                    info.price_after_1s, info.price_after_5s, info.price_after_10s, info.price_after_20s, info.price_after_30s, info.price_after_40s, info.price_after_50s,
                    info.price_after_60s, info.price_after_120s, info.price_after_180s, info.price_after_15min, info.price_after_30min, info.price_after_1hour,
                    info.pnl_1s_percentage, info.pnl_5s_percentage, info.pnl_10s_percentage, info.pnl_20s_percentage, info.pnl_30s_percentage, info.pnl_40s_percentage, info.pnl_50s_percentage,
                    info.pnl_60s_percentage, info.pnl_120s_percentage, info.pnl_180s_percentage, info.pnl_15min_percentage, info.pnl_30min_percentage, info.pnl_1hour_percentage,
                    info.commission_rate, has_trading_volume,
                    info.filled_size, has_trading_volume,
                    info.price_after_1s, info.price_after_5s, info.price_after_10s, info.price_after_20s, info.price_after_30s, info.price_after_40s, info.price_after_50s,
                    info.price_after_60s, info.price_after_120s, info.price_after_180s, info.price_after_15min, info.price_after_30min, info.price_after_1hour,
                    info.pnl_1s_percentage, info.pnl_5s_percentage, info.pnl_10s_percentage, info.pnl_20s_percentage, info.pnl_30s_percentage, info.pnl_40s_percentage, info.pnl_50s_percentage,
                    info.pnl_60s_percentage, info.pnl_120s_percentage, info.pnl_180s_percentage, info.pnl_15min_percentage, info.pnl_30min_percentage, info.pnl_1hour_percentage
                );

                info_log("save or update order stat sql: {}", sql);
                if (my_conn != nullptr) {
                    try {
                        if (mysql_query(my_conn, sql.c_str()) != 0) {
                            int my_no = mysql_errno(my_conn);
                            string my_err = mysql_error(my_conn);
                            err_log("fail to insert or update order: {} {}", my_no, my_err);
                        }
                    } catch (exception &exp) {
                        err_log("exception occur while insert or update order stat: {}", string(exp.what()));
                    }
                } else {
                    warn_log("no mysql connection created");
                }
            }
            if (my_conn != nullptr) {
                context.get_mysql_source()->releaseConnection(my_conn);
            }

            // remove expired order stat info
            pair<int, int> removed = context.get_stat_order_composite().remove_expired_stat_order();
            if (now % 10 == 0) {
                // reduce logging lines
                info_log("finish remove expired stat orders: size reduce from {} to {}", removed.first, removed.second);
            }
        }
    }
}