#include "trade_processor.h"
#include "common/random.h"
#include <thread>
#include <chrono>

namespace trader {

    void start_trade_processors(TraderConfig& config, GlobalContext& context) {
        
        // normal type or bestpath type both use the same message channel
        std::thread thread_process_message(process_order_message, std::ref(config), std::ref(context));
        thread_process_message.detach();
        info_log("start thread of processing normal order message");

        if (!config.trading_use_best_path) {
            // not use best path for trading, require start normal order service
            std::thread thread_order_service(start_order_service, std::ref(config), std::ref(context));
            thread_order_service.detach();
            info_log("start thread of starting normal order serice");
        }

        for (std::string base_asset : config.base_asset_list) {
            std::thread thread_scan(scan_and_send_order, std::ref(config), std::ref(context), base_asset);
            thread_scan.detach();
            info_log("start thread of scanning and sending order for {}", base_asset);
        }
    }

    void scan_and_send_order(TraderConfig& config, GlobalContext& context, std::string base_asset) {
        
        std::string follower_inst_id = base_asset + config.follower_quote_asset;
        int shm_mapping_index = -1;
        auto mapping = context.get_shm_order_mapping().find(follower_inst_id);
        if (mapping == context.get_shm_order_mapping().end()) {
            warn_log("order shm mapping index not found");
            return;
        } else {
            shm_mapping_index = (*mapping).second;
        }

        long order_version = 0;
        RandomIntGen rand;
        rand.init(0, 10000);

        while (true) {

            if (config.loop_pause_time_millis > 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(config.loop_pause_time_millis));
            }

            int rnd_number = rand.randInt();

            shared_ptr<shm_mng::OrderShm> shm_order = shm_mng::order_shm_reader_get(context.get_shm_store_info().order_start, shm_mapping_index);
            if (shm_order == nullptr) {
                if (rnd_number < 10) {
                    warn_log("order in share memory not found for {}", base_asset);
                }
                continue;
            }

            if ((*shm_order).version_number <= order_version) {
                if (rnd_number < 10) {
                    warn_log("order version in share memory is old for {} {} {} {}:{}", base_asset, shm_mapping_index, (*shm_order).inst_id, (*shm_order).version_number, order_version);
                }
                continue;
            }
            order_version = (*shm_order).version_number;

            uint64_t now = binance::get_current_ms_epoch();
            if (now > (*shm_order).update_time + config.order_valid_millis) {
                if (rnd_number < 10) {
                    warn_log("order is expired for {}", base_asset);
                }
                continue;
            }

            binance::FuturesNewOrder order;
            order.symbol = follower_inst_id;
            order.side = std::string((*shm_order).side);
            order.positionSide = std::string((*shm_order).pos_side);
            order.quantity = (*shm_order).volume;
            order.price = (*shm_order).price;
            order.type = std::string((*shm_order).type);
            order.timeInForce = std::string((*shm_order).time_in_force);
            order.newClientOrderId = std::string((*shm_order).client_order_id);
            order.newOrderRespType = binance::ORDER_RESP_TYPE_RESULT;
            // order.reduceOnly = (*shm_order).reduce_only == 1 ? "true" : "false";

            pair<bool, string> result;
            if (config.open_place_order) {
                if (!context.get_api_minute_limiter()->get_semaphore(1) || !context.get_api_second_limiter()->get_semaphore(1)) {
                    warn_log("no more semaphore for place order");
                    result.first = false;
                    result.second = "no semephore";
                } else {
                    if (config.trading_use_best_path) {
                        optional<shared_ptr<WsClientWrapper>> best_service = context.get_order_service_manager().find_best_service(follower_inst_id);
                        if (!best_service.has_value()) {
                            // no available order service
                            warn_log("not found available order service for {}", follower_inst_id);
                            result.first = false;
                            result.second = "no order service";
                        } else {
                            result = best_service.value()->place_order(order);
                        }
                    } else {
                        result = context.get_order_service().placeOrder(order);
                    }
                }
            } else {
                result.first = false;
                result.second = "config stop";
            }
            
            info_log("place order: result={} msg={} order(inst_id={} side={} pos_side={} price={} volume={} client_id={} reduce_only={})",
                result.first, result.second, order.symbol, order.side, order.positionSide, order.price, order.quantity, order.newClientOrderId, order.reduceOnly);
        }
    }

    void start_order_service(TraderConfig& config, GlobalContext& context) {
        while (true) {
            try {
                pair<bool, string> result = context.get_order_service().startOrderService();
            } catch (std::exception &e) {
                err_log("fail to start order service: {}", std::string(e.what()));
            }

            // reconnect after 5 seconds
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
    }

    void process_order_message(TraderConfig& config, GlobalContext& context) {
        shared_ptr<moodycamel::ConcurrentQueue<std::string>> channel = context.get_order_channel();

        RandomIntGen rand;
        rand.init(0, 10000);

        while (true) {
            std::string messageJson;
            while (!channel->try_dequeue(messageJson)) {
                // Retry if the queue is empty
                std::this_thread::sleep_for(std::chrono::milliseconds(3));
            }

            try {
                // process message
                Json::Value json_result;
                Json::Reader reader;
                json_result.clear();
                reader.parse(messageJson.c_str(), json_result);

                info_log("receive order message: {}", messageJson);

                binance::WsFuturesOrderCallbackEvent event = binance::convertJsonToWsFuturesOrderCallbackEvent(json_result);
                if (event.status == binance::WsCallbackStatusOK) {
                    info_log("receive order event: inst_id={} client_id={} status={} side={} position_side={} orig_qty={} executed_qty={} price={}",
                        event.result.symbol, event.result.clientOrderId, event.result.status,
                        event.result.side, event.result.positionSide, event.result.origQty,
                        event.result.executedQty, event.result.price
                    );
                } else {
                    err_log("error occur receive order event: id={} code={} msg={}", event.id, event.error.code, event.error.msg);
                }

            } catch (std::exception &exp) {
                err_log("fail to process order message: {}", std::string( exp.what()));
            }
        }
    }
}