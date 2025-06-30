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

        for (std::string base_asset : config.node_base_assets) {
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
        RandomIntGen log_rand;
        log_rand.init(0, 100000);

        while (true) {

            if (config.loop_pause_time_millis > 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(config.loop_pause_time_millis));
            }

            int log_rand_number = log_rand.randInt();

            shared_ptr<shm_mng::OrderShm> shm_order = shm_mng::order_shm_reader_get(context.get_shm_store_info().order_start, shm_mapping_index);
            if (shm_order == nullptr) {
                if (log_rand_number < 10) {
                    warn_log("order in share memory not found for {}", base_asset);
                }
                continue;
            }

            if ((*shm_order).version_number <= order_version) {
                if (log_rand_number < 10) {
                    // warn_log("order version in share memory is old for {} {} {} {}:{}", base_asset, shm_mapping_index, (*shm_order).inst_id, (*shm_order).version_number, order_version);
                }
                continue;
            }
            order_version = (*shm_order).version_number;

            uint64_t now = binance::get_current_ms_epoch();
            int order_delay_millis = 0;

            if (now > (*shm_order).update_time + config.order_validity_millis) {
                warn_log("order is expired for {} {} {}", base_asset, shm_order->client_order_id, now - shm_order->update_time);
                continue;
            } else {
                if (now > shm_order->update_time) {
                    order_delay_millis = int(now - shm_order->update_time);
                }
            }

            string client_order_id = std::string(shm_order->client_order_id) + "_" + std::to_string(order_delay_millis);

            binance::FuturesNewOrder order;
            order.symbol = follower_inst_id;
            order.side = std::string((*shm_order).side);
            order.positionSide = std::string((*shm_order).pos_side);
            order.quantity = (*shm_order).volume;
            order.price = (*shm_order).price;
            order.type = std::string((*shm_order).type);
            order.timeInForce = std::string((*shm_order).time_in_force);
            order.newClientOrderId = client_order_id;
            order.newOrderRespType = binance::ORDER_RESP_TYPE_RESULT;
            // there no need to send reduceOnly, because if oversold occurs, it means that the current market has revered.
            // order.reduceOnly = (*shm_order).reduce_only == 1 ? "true" : "false";

            bool has_interval_semaphore = context.get_order_interval_boss()->has_more_semaphore(follower_inst_id);
            pair<bool, string> result;

            if (config.open_place_order && has_interval_semaphore) {

                if (config.trading_use_best_path) {
                    optional<shared_ptr<WsClientWrapper>> best_service = context.get_best_order_service_manager().find_best_service(follower_inst_id);
                    if (!best_service.has_value()) {
                        // no available order service
                        warn_log("not found available order service for {}", follower_inst_id);
                        result.first = false;
                        result.second = "no best service";
                    } else {
                        pair<bool, bool> has_semaphore = context.get_order_best_path_limiter()->get_order_semaphore(best_service.value()->get_local_ip(), 1);
                        if (!has_semaphore.first || !has_semaphore.second) {
                            warn_log("no more semaphore for place order with best ip {} account={}:ip={}",
                                best_service.value()->get_local_ip(),
                                has_semaphore.first, has_semaphore.second
                            );
                            result.first = false;
                            result.second = "no semaphore";
                        } else {
                            result = best_service.value()->place_order(order);
                        }
                    }
                } else {
                    if (!context.get_order_normal_minute_limiter()->get_semaphore(1) || !context.get_order_normal_second_limiter()->get_semaphore(1)) {
                        warn_log("no more semaphore for place order");
                        result.first = false;
                        result.second = "no semaphore";
                    } else {
                        result = context.get_normal_order_service().placeOrder(order);
                    }
                }
            } else {
                result.first = false;
                result.second = "config-stop/no-interval";
            }
        
            info_log("place order: result={} msg={} has_interval_semaphore={} order(inst_id={} side={} pos_side={} price={} volume={} client_id={} reduce_only={})",
                result.first, result.second, has_interval_semaphore, order.symbol, order.side, order.positionSide, order.price, order.quantity, order.newClientOrderId, order.reduceOnly);
        }
    }

    void start_order_service(TraderConfig& config, GlobalContext& context) {
        while (true) {
            try {
                pair<bool, string> result = context.get_normal_order_service().startOrderService();
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

                    bool burn_interval_semaphore = false;
                    if (event.result.status == binance::ORDER_STATUS_FILLED || event.result.status == binance::ORDER_STATUS_PARTIALLY_FILLED) {
                        // burn a semaphore of place order interval, to pause palce order with the same symbol for a while
                        burn_interval_semaphore = context.get_order_interval_boss()->burn_semaphore(event.result.symbol);
                    }

                    info_log("receive order event: inst_id={} client_id={} status={} side={} position_side={} orig_qty={} executed_qty={} price={} burn_interval_semaphore={}",
                        event.result.symbol, event.result.clientOrderId, event.result.status,
                        event.result.side, event.result.positionSide, event.result.origQty,
                        event.result.executedQty, event.result.price, burn_interval_semaphore
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