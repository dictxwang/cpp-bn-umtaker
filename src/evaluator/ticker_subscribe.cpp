#include "ticker_subscribe.h"

namespace evaluator {

    void start_subscribe_normal_ticker(EvaluatorConfig& config, GlobalContext& context) {
        
        std::thread process_follower(process_normal_futures_ticker_message, std::ref(config), std::ref(context));
        process_follower.detach();

        std::thread subscribe_follower(subscribe_normal_futures_ticker, std::ref(config), std::ref(context));
        subscribe_follower.detach();
    }

    void process_normal_futures_ticker_message(EvaluatorConfig& config, GlobalContext& context) {
        
        shared_ptr<moodycamel::ConcurrentQueue<string>> channel = context.get_futures_ticker_channel();

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

                int rand_value = rand.randInt();

                TickerLite lite;
                lite.symbol = event.symbol;
                lite.ask = event.bestAskPrice;
                lite.bid = event.bestBidPrice;
                lite.update_id = event.updateId;
                lite.update_time_millis = event.eventTime;

                context.get_futures_ticker_composite().update_ticker(lite);

                if (rand_value < 5) {
                    info_log("update normal futures ticker: symbol={} bid={} ask={} update_id={} update_time={}",
                        lite.symbol, lite.bid, lite.ask, lite.update_id, lite.update_time_millis
                    );
                }

             } catch (std::exception &exp) {
                err_log("fail to process normal futures ticker message: {}", std::string( exp.what()));
            }
        }
    }

    void subscribe_normal_futures_ticker(EvaluatorConfig& config, GlobalContext& context) {

        binance::BinanceFuturesWsClient futuresWsClient;

        if (config.normal_ticker_local_ip != "") {
            futuresWsClient.setLocalIP(config.normal_ticker_local_ip);
        }

        futuresWsClient.initBookTickerV1(false, false);
        futuresWsClient.setMessageChannel(context.get_futures_ticker_channel());

        while (true) {
            std::pair<bool, string> result;
            try {
                result = futuresWsClient.startAllBookTickersV1();
            } catch (std::exception &exp) {
                err_log("error occur while book ticker: {}", std::string(exp.what()));
            }

            err_log("stop book futures ticker subscribe: {}", result.second);

            // wait for a while after exception
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
    }

}