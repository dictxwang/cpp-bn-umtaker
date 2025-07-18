#include "zmq_subscribe.h"

namespace evaluator {

    void start_order_stat_zmq(EvaluatorConfig& config, GlobalContext& context) {

        for (size_t i = 0; i < config.order_stat_zmq_ipcs.size(); ++i) {
            thread sub_zmq_thread(subscribe_process_order_stat_zmq, ref(config), ref(context), i);
            sub_zmq_thread.detach();
            info_log("start subscribe order stat zmq {} thread", config.order_stat_zmq_ipcs[i]);
        }
    }

    void subscribe_process_order_stat_zmq(EvaluatorConfig& config, GlobalContext& context, size_t ipc_index) {

        this_thread::sleep_for(chrono::seconds(5));
        string order_stat_zmq_ipc = config.order_stat_zmq_ipcs[ipc_index];
        while (true) {

            ZMQClient zmq_client(ZMQ_SUB);
            try {
                zmq_client.SubscriberConnect(order_stat_zmq_ipc);
            } catch (std::exception &exp) {
                err_log("error occur while zmq subscribe connection to {}: {}", order_stat_zmq_ipc, std::string(exp.what()));
            }

            info_log("success subscribe order stat zmq message");

            while (true) {
                try {
                    std::string message = zmq_client.Receive();
                    strHelper::replaceString(message, "\n", "");
                    info_log("receive order stat zmq message: {}", message);

                    Json::Value json_result;
                    Json::Reader reader;
                    json_result.clear();
                    reader.parse(message, json_result);

                    StatOrderLite lite;
                    lite.accountFlag = json_result["accountFlag"].asString();
                    lite.symbol = json_result["symbol"].asString();
                    lite.orderSide = json_result["orderSide"].asString();
                    lite.clientOrderId = json_result["clientOrderId"].asString();
                    lite.averagePrice = binance::str_to_dobule(json_result["averagePrice"]);
                    lite.filledSize = binance::str_to_dobule(json_result["filledSize"]);
                    lite.commissionRate = binance::str_to_dobule(json_result["commissionRate"]);
                    lite.systemTimestamp = json_result["systemTimestamp"].asUInt64();

                    bool updated = context.get_stat_order_composite().update_with_lite(lite);

                    info_log("update stat order: {} {} {} {} {} {} {} {} {}", lite.accountFlag, lite.symbol, lite.orderSide, lite.clientOrderId, lite.averagePrice, lite.filledSize, lite.commissionRate, lite.systemTimestamp, updated);
                } catch (exception &exp) {
                    err_log("error occur while parse zmq message");
                    break;
                }
            }

            err_log("will re-subscribe zmq order stat after 5 secs");

            // wait for a while after exception
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
    }
}