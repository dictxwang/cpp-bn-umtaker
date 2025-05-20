#include <thread>
#include <chrono>
#include "ticker_subscribe.h"
#include "binancecpp/binance_ws_futures.h"
#include "logger/logger.h"

namespace receiver {

    void start_subscribe_normal_ticker(ReceiverConfig& config, GlobalContext& context) {

        std::thread sub_benchmark(subscribe_normal_ticker, config, context, context.get_benchmark_inst_ids());
        std::thread sub_follower(subscribe_normal_ticker, config, context, context.get_follower_inst_ids());
    }

    void start_subscribe_best_ticker(ReceiverConfig& config, GlobalContext& context) {

    }


    void subscribe_normal_ticker(ReceiverConfig& config, GlobalContext& context, vector<string> &inst_ids) {

        while (true) {
            binance::BinanceFuturesWsClient futuresWsClient;
            if (config.normal_ticker_local_ip != "") {
                futuresWsClient.setLocalIP(config.normal_ticker_local_ip);
            }
            futuresWsClient.initBookTickerV1(config.normal_ticker_use_intranet, false);
            try {
                futuresWsClient.startBookTickerV1(processFuturesTickerMessage, inst_ids);
            } catch (std::exception &exp) {
                err_log("error occur while book ticker: {}", exp.what());
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