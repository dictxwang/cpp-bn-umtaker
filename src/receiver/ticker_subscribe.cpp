#include <thread>
#include <chrono>
#include "ticker_subscribe.h"
#include "logger/logger.h"
#include "common/common.h"
#include "common/tools.h"

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

    void start_subscribe_best_ticker(ReceiverConfig& config, GlobalContext& context) {

    }

    void process_normal_ticker_message(ReceiverConfig& config, GlobalContext &context, TickerRole role) {
        moodycamel::ConcurrentQueue<string> *channel;
        if (role == TickerRole::Benchmark) {
            channel = context.get_benchmark_ticker_channel();
        } else {
            channel = context.get_follower_ticker_channel();
        }

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
                info.update_time_millis = event.eventTime;
                info.is_from_trade = false;

                if (role == TickerRole::Benchmark) {
                    std::cout << "ticker for benchmark: " << event.symbol << std::endl;
                    context.get_benchmark_ticker_composite().update_ticker(info);
                } else {
                    std::cout << "ticker for follower: " << event.symbol << std::endl;
                    context.get_follower_ticker_composite().update_ticker(info);
                }
                std::string format = "process normal ticker for";
                info_log(format.c_str());
            } catch (std::exception &exp) {
                std::cout << "exception occur: " << exp.what() << std::endl;
                //err_log("fail to process normal ticker message: {}", std::string( exp.what()));
            }
        }
    }

    void subscribe_normal_ticker(ReceiverConfig &config, GlobalContext& context, vector<string> &inst_ids, TickerRole role) {

        while (true) {
            binance::BinanceFuturesWsClient futuresWsClient;
            if (config.normal_ticker_local_ip != "") {
                futuresWsClient.setLocalIP(config.normal_ticker_local_ip);
            }
            futuresWsClient.initBookTickerV1(config.normal_ticker_use_intranet, false);
            try {
                if (role == TickerRole::Benchmark) {
                    futuresWsClient.setMessageChannel(context.get_benchmark_ticker_channel());
                } else {
                    futuresWsClient.setMessageChannel(context.get_follower_ticker_channel());
                }
                futuresWsClient.startBookTickerV1(inst_ids);
            } catch (std::exception &exp) {
                //err_log("error occur while book ticker: {}", std::string(exp.what()));
            }

            // wait for a while after exception
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
    }

    void subscribe_best_ticker(GlobalContext& context, string &ipc) {

    }


    bool processFuturesTickerMessage(std::string &messageJson) {
        Json::Value json_result;
        Json::Reader reader;
        json_result.clear();
        reader.parse(messageJson.c_str(), json_result);

        if (json_result.isMember("data")) {
            json_result = json_result["data"];
        }
        binance::WsFuturesBookTickerEvent event = binance::convertJsonToWsFuturesBookTickerEvent(json_result);
        // TODO
        std::cout << "convert ticker event for : " << event.symbol << std::endl;
        return true;
    }

}