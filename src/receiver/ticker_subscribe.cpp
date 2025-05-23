#include <thread>
#include <chrono>
#include "ticker_subscribe.h"

namespace receiver {

    void start_subscribe_normal_ticker(ReceiverConfig& config, GlobalContext& context) {

        std::thread process_ticker_price_offset(process_ticker_info_price_offset, std::ref(config), std::ref(context));
        process_ticker_price_offset.detach();

        std::thread process_benchmark(process_normal_ticker_message, std::ref(config), std::ref(context), TickerRole::Benchmark);
        process_benchmark.detach();
        std::thread process_follower(process_normal_ticker_message, std::ref(config), std::ref(context), TickerRole::Follower);
        process_follower.detach();

        std::thread subscribe_benchmark(subscribe_normal_ticker, std::ref(config), std::ref(context), std::ref(context.get_benchmark_inst_ids()), TickerRole::Benchmark);
        subscribe_benchmark.detach();
        std::thread subscribe_follower(subscribe_normal_ticker, std::ref(config), std::ref(context), std::ref(context.get_follower_inst_ids()), TickerRole::Follower);
        subscribe_follower.detach();
    }

    void process_ticker_info_price_offset(ReceiverConfig& config, GlobalContext &context) {
        
        RandomIntGen rand;
        rand.init(0, 10000);

        while (true) {
            UmTickerInfo ticker_info;
            while (!context.get_ticker_info_channel()->try_dequeue(ticker_info)) {
                // Retry if the queue is empty
            }

            if (str_ends_with(ticker_info.inst_id, config.benchmark_quote_asset)) {
                std::string base_asset = ticker_info.inst_id.substr(0, ticker_info.inst_id.size() - config.benchmark_quote_asset.size());
                std::string follower_inst_id = base_asset + config.follower_quote_asset;
                optional<UmTickerInfo> follower_ticker_info = context.get_follower_ticker_composite().get_lastest_ticker(follower_inst_id);
                if (follower_ticker_info.has_value()) {
                    context.get_price_offset_composite().update_price_offset(base_asset, ticker_info, follower_ticker_info.value());
                }
            } else if (str_ends_with(ticker_info.inst_id, config.follower_quote_asset)) {
                std::string base_asset = ticker_info.inst_id.substr(0, ticker_info.inst_id.size() - config.follower_quote_asset.size());
                std::string benchmark_inst_id = base_asset + config.benchmark_quote_asset;
                optional<UmTickerInfo> benchmark_ticker_info = context.get_benchmark_ticker_composite().get_lastest_ticker(benchmark_inst_id);
                if (benchmark_ticker_info.has_value()) {
                    context.get_price_offset_composite().update_price_offset(base_asset, benchmark_ticker_info.value(), ticker_info);
                }
            }

            if (rand.randInt() < 100) {
                info_log("process ticker for price offset: symbol={} bid={} bid_size={} ask={} ask_size={} up_ts={}",
                    ticker_info.inst_id, ticker_info.bid_price, ticker_info.bid_volume, ticker_info.ask_price, ticker_info.ask_volume, ticker_info.update_time_millis);
            }
        }
    } 

    void start_subscribe_zmq_best_ticker(ReceiverConfig& config, GlobalContext& context) {

        for (std::string ipc : config.ticker_zmq_ipcs) {
            std::thread subscribe_and_process(subscribe_process_zmq_best_ticker, std::ref(config), std::ref(context), std::ref(ipc));
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

                if (role == TickerRole::Benchmark) {
                    // std::cout << "ticker for benchmark: " << event.symbol << std::endl;
                    context.get_benchmark_ticker_composite().update_ticker(info);
                } else {
                    // std::cout << "ticker for follower: " << event.symbol << std::endl;
                    context.get_follower_ticker_composite().update_ticker(info);
                }

                // put info queue for price offset
                bool result = (*context.get_ticker_info_channel()).try_enqueue(info);
                if (!result) {
                    std::cout << "can not enqueue ticker info: " << messageJson << std::endl;
                }

                if (rand.randInt() < 100) {
                    info_log("process normal ticker: symbol={} bid={} bid_size={} ask={} ask_size={} up_ts={}",
                        info.inst_id, info.bid_price, info.bid_volume, info.ask_price, info.ask_volume, info.update_time_millis);
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

    void subscribe_process_zmq_best_ticker(ReceiverConfig& config, GlobalContext& context, string &ipc) {

        while (true) {
            ZMQClient zmq_client(ZMQ_SUB);
            try {
                zmq_client.SubscriberConnect(ipc);
            } catch (std::exception &exp) {
                err_log("error occur while zmq subscribe connection");
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

                    if (str_ends_with(info.inst_id, config.benchmark_quote_asset)) {
                        // std::cout << "ticker for benchmark: " << event.symbol << std::endl;
                        context.get_benchmark_ticker_composite().update_ticker(info);
                    } else {
                        // std::cout << "ticker for follower: " << event.symbol << std::endl;
                        context.get_follower_ticker_composite().update_ticker(info);
                    }
                    if (rand.randInt() < 100) {
                        info_log("process zmq best ticker: symbol={} bid={} bid_size={} ask={} ask_size={} up_ts={}",
                            info.inst_id, info.bid_price, info.bid_volume, info.ask_price, info.ask_volume, info.update_time_millis);
                    }
                } catch (std::exception &exp) {
                    err_log("error occur while parse zmq message");
                    break;
                }
            }
        }
    }
}