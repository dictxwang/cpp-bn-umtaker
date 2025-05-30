#include <thread>
#include <chrono>
#include "ticker_subscribe.h"

namespace receiver {

    void start_subscribe_normal_ticker(ReceiverConfig& config, GlobalContext& context) {

        std::thread process_benchmark(process_normal_ticker_message, std::ref(config), std::ref(context), TickerRole::Benchmark);
        process_benchmark.detach();
        std::thread process_follower(process_normal_ticker_message, std::ref(config), std::ref(context), TickerRole::Follower);
        process_follower.detach();

        std::thread subscribe_benchmark(subscribe_normal_ticker, std::ref(config), std::ref(context), std::ref(context.get_benchmark_inst_ids()), TickerRole::Benchmark);
        subscribe_benchmark.detach();
        std::thread subscribe_follower(subscribe_normal_ticker, std::ref(config), std::ref(context), std::ref(context.get_follower_inst_ids()), TickerRole::Follower);
        subscribe_follower.detach();
    }

    void start_subscribe_zmq_best_ticker(ReceiverConfig& config, GlobalContext& context) {

        for (size_t i = 0; i < config.ticker_zmq_ipcs.size(); ++i) {
            std::thread subscribe_and_process(subscribe_process_zmq_best_ticker, std::ref(config), std::ref(context), i);
            subscribe_and_process.detach();
        }
    }

    void process_normal_ticker_message(ReceiverConfig& config, GlobalContext &context, TickerRole role) {
        moodycamel::ConcurrentQueue<string> *channel;
        if (role == TickerRole::Benchmark) {
            channel = context.get_benchmark_ticker_channel();
        } else {
            channel = context.get_follower_ticker_channel();
        }

        RandomIntGen rand;
        rand.init(0, 10000);

        while (true) {
            std::string messageJson;
            while (!channel->try_dequeue(messageJson)) {
                // Retry if the queue is empty
            }

            try {
                // process message
                Json::Value json_result;
                Json::Reader reader;
                json_result.clear();
                reader.parse(messageJson.c_str(), json_result);

                if (json_result.isMember("data")) {
                    json_result = json_result["data"];
                }
                binance::WsFuturesBookTickerEvent event = binance::convertJsonToWsFuturesBookTickerEvent(json_result);

                UmTickerInfo info;
                info.inst_id = event.symbol;
                info.bid_price = event.bestBidPrice;
                info.bid_volume = event.bestBidQty;
                info.ask_price = event.bestAskPrice;
                info.ask_volume = event.bestAskQty;
                info.avg_price = (event.bestBidPrice + event.bestAskPrice) / 2;
                info.update_time_millis = event.eventTime;
                info.is_from_trade = false;

                shm_mng::TickerInfoShm info_shm;
                info_shm.bid_price = info.bid_price;
                info_shm.bid_size = info.bid_volume;
                info_shm.ask_price = info.ask_price;
                info_shm.ask_size = info.ask_volume;
                info_shm.update_id = 0;
                info_shm.update_time = info.update_time_millis;

                int rand_value = rand.randInt();
                int update_shm = 0;
                if (role == TickerRole::Benchmark) {
                    // std::cout << "ticker for benchmark: " << event.symbol << std::endl;
                    context.get_benchmark_ticker_composite().update_ticker(info);
                    auto address = context.get_shm_benchmark_ticker_mapping().find(info.inst_id);
                    if (address != context.get_shm_benchmark_ticker_mapping().end()) {
                        update_shm = shm_mng::ticker_shm_writer_update(context.get_shm_store_info().benchmark_start, (*address).second, info_shm);

                        if (rand_value < 20) {
                            std::shared_ptr<shm_mng::TickerInfoShm> ticker = shm_mng::ticker_shm_reader_get(context.get_shm_store_info().benchmark_start, (*address).second);
                            info_log("get benchmark shm normal ticker: {}, {}, {}, {}", (*ticker).inst_id, (*ticker).bid_price, (*ticker).version_number, (*ticker).update_time);
                        }
                    }
                } else {
                    // std::cout << "ticker for follower: " << event.symbol << std::endl;
                    context.get_follower_ticker_composite().update_ticker(info);
                    auto address = context.get_shm_follower_ticker_mapping().find(info.inst_id);
                    if (address != context.get_shm_follower_ticker_mapping().end()) {
                        update_shm = shm_mng::ticker_shm_writer_update(context.get_shm_store_info().follower_start, (*address).second, info_shm);

                        if (rand_value < 20) {
                            std::shared_ptr<shm_mng::TickerInfoShm> ticker = shm_mng::ticker_shm_reader_get(context.get_shm_store_info().follower_start, (*address).second);
                            info_log("get follower shm normal ticker: {}, {}, {}, {}", (*ticker).inst_id, (*ticker).bid_price, (*ticker).version_number, (*ticker).update_time);
                        }
                    }
                }

                // put info queue for price offset
                bool result = (*context.get_ticker_info_channel()).try_enqueue(info);
                if (!result) {
                    warn_log("can not enqueue ticker info: {}", messageJson);
                }

                result = (*context.get_beta_calculation_channel()).try_enqueue(info.inst_id);
                if (!result) {
                    if (rand_value < 10) {
                        warn_log("can not enqueue beta calculation: {}", info.inst_id);
                    }
                }

                if (rand_value < 20) {
                    info_log("process normal ticker: role={} symbol={} bid={} bid_size={} ask={} ask_size={} up_ts={} update_shm={}",
                        strHelper::toString(role), info.inst_id, info.bid_price, info.bid_volume, info.ask_price, info.ask_volume, info.update_time_millis, update_shm);
                }
            } catch (std::exception &exp) {
                err_log("fail to process normal ticker message: {}", std::string( exp.what()));
            }
        }
    }

    void subscribe_normal_ticker(ReceiverConfig &config, GlobalContext& context, vector<string> &inst_ids, TickerRole role) {


        binance::BinanceFuturesWsClient futuresWsClient;

        if (config.normal_ticker_local_ip != "") {
            futuresWsClient.setLocalIP(config.normal_ticker_local_ip);
        }

        futuresWsClient.initBookTickerV1(config.normal_ticker_use_intranet, false);

        if (role == TickerRole::Benchmark) {
            futuresWsClient.setMessageChannel(context.get_benchmark_ticker_channel());
        } else {
            futuresWsClient.setMessageChannel(context.get_follower_ticker_channel());
        }

        while (true) {
            std::pair<bool, string> result;
            try {
                result = futuresWsClient.startBookTickerV1(inst_ids);
            } catch (std::exception &exp) {
                err_log("error occur while book ticker: {}", std::string(exp.what()));
            }

            err_log("stop book ticker subscribe: {}", result.second);

            // wait for a while after exception
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
    }

    void subscribe_process_zmq_best_ticker(ReceiverConfig& config, GlobalContext& context, size_t ipc_index) {

        while (true) {
            ZMQClient zmq_client(ZMQ_SUB);
            try {
                zmq_client.SubscriberConnect(config.ticker_zmq_ipcs[ipc_index]);
            } catch (std::exception &exp) {
                err_log("error occur while zmq subscribe connection: {}", std::string(exp.what()));
            }

            RandomIntGen rand;
            rand.init(0, 10000);

            while (true) {
                try {
                    std::string message = zmq_client.Receive();
                    besttick::TickerInfo event;
                    event.ParseFromString(message);

                    UmTickerInfo info;
                    info.inst_id = event.instid();
                    info.bid_price = event.bestbid();
                    info.bid_volume = event.bidsz();
                    info.ask_price = event.bestask();
                    info.ask_volume = event.asksz();
                    info.avg_price = (event.bestbid() + event.bestask()) / 2;
                    info.update_time_millis = event.eventts();
                    info.is_from_trade = false;

                    shm_mng::TickerInfoShm info_shm;
                    info_shm.bid_price = info.bid_price;
                    info_shm.bid_size = info.bid_volume;
                    info_shm.ask_price = info.ask_price;
                    info_shm.ask_size = info.ask_volume;
                    info_shm.update_id = event.updateid();
                    info_shm.update_time = info.update_time_millis;

                    int rand_value = rand.randInt();
                    int update_shm = 0;

                    if (str_ends_with(info.inst_id, config.benchmark_quote_asset)) {
                        // std::cout << "ticker for benchmark: " << event.symbol << std::endl;
                        context.get_benchmark_ticker_composite().update_ticker(info);

                        auto address = context.get_shm_benchmark_ticker_mapping().find(info.inst_id);
                        if (address != context.get_shm_benchmark_ticker_mapping().end()) {
                            update_shm = shm_mng::ticker_shm_writer_update(context.get_shm_store_info().benchmark_start, (*address).second, info_shm);

                            if (rand_value < 20) {
                                std::shared_ptr<shm_mng::TickerInfoShm> ticker = shm_mng::ticker_shm_reader_get(context.get_shm_store_info().benchmark_start, (*address).second);
                                info_log("get benchmark shm zmq ticker: {}, {}, {}, {}", (*ticker).inst_id, (*ticker).bid_price, (*ticker).version_number, (*ticker).update_time);
                            }
                        }
                    } else {
                        // std::cout << "ticker for follower: " << event.symbol << std::endl;
                        context.get_follower_ticker_composite().update_ticker(info);
                       
                        auto address = context.get_shm_follower_ticker_mapping().find(info.inst_id);
                        if (address != context.get_shm_follower_ticker_mapping().end()) {
                            update_shm = shm_mng::ticker_shm_writer_update(context.get_shm_store_info().follower_start, (*address).second, info_shm);
    
                            if (rand_value < 20) {
                                std::shared_ptr<shm_mng::TickerInfoShm> ticker = shm_mng::ticker_shm_reader_get(context.get_shm_store_info().follower_start, (*address).second);
                                info_log("get follower shm zmq ticker: {}, {}, {}, {}", (*ticker).inst_id, (*ticker).bid_price, (*ticker).version_number, (*ticker).update_time);
                            }
                        }
                    }

                    // put info queue for price offset
                    bool result = (*context.get_ticker_info_channel()).try_enqueue(info);
                    if (!result) {
                        warn_log("can not enqueue ticker info: {}", info.inst_id);
                    }

                    if (rand_value < 20) {
                        info_log("process zmq best ticker: symbol={} bid={} bid_size={} ask={} ask_size={} up_ts={} update_shm={}",
                            info.inst_id, info.bid_price, info.bid_volume, info.ask_price, info.ask_volume, info.update_time_millis, update_shm);
                    }
                } catch (std::exception &exp) {
                    err_log("error occur while parse zmq message");
                    break;
                }
            }
        }
    }
}