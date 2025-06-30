#include "strategy_processor.h"

#include <cstdlib> // For exit()
#include <thread>
#include <algorithm>

namespace actuary {

    void start_strategy_processors(ActuaryConfig& config, GlobalContext& context) {

        for (size_t i = 0; i < config.node_base_assets.size(); ++i) {

            std::string base_asset = config.node_base_assets[i];
            std::thread check_task_thread(calculate_signal_make_order, std::ref(config), std::ref(context), base_asset);
            check_task_thread.detach();
            info_log("start strategy processor for {}", base_asset);
        }
    }

    void calculate_signal_make_order(ActuaryConfig& config, GlobalContext& context, std::string base_asset) {

        std::string benchmark_inst_id = base_asset + config.benchmark_quote_asset;
        std::string follower_inst_id = base_asset + config.follower_quote_asset;

        optional<ExchangeInfoLite> exchange_opt = context.get_exchange_info(follower_inst_id);
        if (!exchange_opt.has_value()) {
            err_log("exchange info not found for {}", follower_inst_id);
            this_thread::sleep_for(chrono::seconds(3));
            exit(-1);
        }
        ExchangeInfoLite follower_exchange_info = exchange_opt.value();

        int benchmark_shm_index = -1;
        int follower_shm_index = -1;
        int threshold_shm_index = -1;
        int order_shm_index = -1;

        auto benchmark = context.get_shm_benchmark_ticker_mapping().find(benchmark_inst_id);
        if (benchmark != context.get_shm_benchmark_ticker_mapping().end()) {
            benchmark_shm_index = (*benchmark).second;
        }

        auto follower = context.get_shm_follower_ticker_mapping().find(follower_inst_id);
        if (follower != context.get_shm_follower_ticker_mapping().end()) {
            follower_shm_index = (*follower).second;
        }

        auto threshold = context.get_shm_threshold_mapping().find(base_asset);
        if (threshold != context.get_shm_threshold_mapping().end()) {
            threshold_shm_index = (*threshold).second;
        }

        auto order = context.get_shm_order_mapping().find(follower_inst_id);
        if (order != context.get_shm_order_mapping().end()) {
            order_shm_index = (*order).second;
        }

        info_log("find shm mapping index for {}: benchmark_shm_index={} follower_shm_index={} threshold_shm_index={} order_shm_index={}",
            base_asset, benchmark_shm_index, follower_shm_index, threshold_shm_index, order_shm_index
        );
        if (benchmark_shm_index < 0 || follower_shm_index < 0 || threshold_shm_index < 0 || order_shm_index < 0) {
            err_log("fail to find shm mapping index for {}", base_asset);
            this_thread::sleep_for(chrono::seconds(3));
            exit(-1);
        }

        auto inst_config_auto = context.get_follower_inst_config().inst_map.find(base_asset);
        if (inst_config_auto == context.get_follower_inst_config().inst_map.end()) {
            warn_log("inst config not found for {}", base_asset);
            this_thread::sleep_for(chrono::seconds(3));
            exit(-1);
        }

        double latest_buy_order_price = 0;
        uint64_t latest_buy_order_millis = 0;
        double latest_sell_order_price = 0;
        uint64_t latest_sell_order_millis = 0;

        RandomIntGen rand_log;
        rand_log.init(0, 500000);
        RandomIntGen rand_order_log;
        rand_order_log.init(0, 100);

        InstConfigItem inst_config = (*inst_config_auto).second;
        long benchmark_ticker_version, follower_ticker_version, early_run_version, benchmark_beta_version, follower_beta_version = 0;

        std::this_thread::sleep_for(std::chrono::seconds(10));
        info_log("sleep a while before signal calculation");

        while (true) {

            if (config.loop_pause_time_millis > 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(config.loop_pause_time_millis));
            }
            int rand_log_number = rand_log.randInt();
            int rand_order_log_number = rand_order_log.randInt();
            uint64_t now = binance::get_current_ms_epoch();
            int ticker_delay_millis = 0;

            std::shared_ptr<shm_mng::TickerInfoShm> benchmark_ticker = shm_mng::ticker_shm_reader_get(context.get_shm_store_info().benchmark_start, benchmark_shm_index);
            std::shared_ptr<shm_mng::TickerInfoShm> follower_ticker = shm_mng::ticker_shm_reader_get(context.get_shm_store_info().follower_start, follower_shm_index);
            if (benchmark_ticker == nullptr || follower_ticker == nullptr) {
                if (rand_log_number < 10) {
                    warn_log("ticker in share memory not found for {}", base_asset);
                }
                continue;
            }
            if ((*benchmark_ticker).version_number == 0 || (*follower_ticker).version_number == 0) {
                if (rand_log_number < 10) {
                    warn_log("ticker version in share memory are zero {}", base_asset);
                }
                continue;
            }

            long benchmark_ticker_new_version = benchmark_ticker->version_number;
            long follower_ticker_new_version = follower_ticker->version_number;
            if (benchmark_ticker_new_version < benchmark_ticker_version || follower_ticker_new_version < follower_ticker_version) {
                if (rand_log_number < 10) {
                    warn_log("ticker version is old in share memory for {}", base_asset);
                }
                continue;
            }

            bool benchmark_ticker_changed, follower_ticker_changed = false;
            if (benchmark_ticker_new_version > benchmark_ticker_version) {
                benchmark_ticker_version = benchmark_ticker_new_version;
                benchmark_ticker_changed = true;
            }
            if (follower_ticker_new_version > follower_ticker_version) {
                follower_ticker_version = follower_ticker_new_version;
                follower_ticker_changed = true;
            }

            if ((!config.enable_benchmark_ticker_trigger || !benchmark_ticker_changed)
                && (!config.enable_follower_ticker_trigger || !follower_ticker_changed)) {
                if (rand_log_number < 10) {
                    warn_log("enabled trigger ticker version is not change in share memory for {}", base_asset);
                }
                continue;
            }

            std::shared_ptr<shm_mng::EarlyRunThresholdShm> early_run_threshold = shm_mng::early_run_shm_reader_get(context.get_shm_store_info().early_run_start, threshold_shm_index);
            std::shared_ptr<shm_mng::BetaThresholdShm> benchmark_beta_threshold = shm_mng::beta_shm_reader_get(context.get_shm_store_info().benchmark_beta_start, threshold_shm_index);
            std::shared_ptr<shm_mng::BetaThresholdShm> follower_beta_threshold = shm_mng::beta_shm_reader_get(context.get_shm_store_info().follower_beta_start, threshold_shm_index);
            
            if (early_run_threshold == nullptr || (*early_run_threshold).version_number == 0) {
                if (rand_log_number < 10) {
                    warn_log("threshold early run in share memory not found or version is zero for {}", base_asset);
                }
                continue;
            }
            if (benchmark_beta_threshold == nullptr || benchmark_beta_threshold->version_number == 0) {
                if (rand_log_number < 10) {
                    warn_log("benchmark threshold beta in share memory not found or version is zero for {}", base_asset);
                }
                continue;
            }
            if (follower_beta_threshold == nullptr || follower_beta_threshold->version_number == 0) {
                if (rand_log_number < 10) {
                    warn_log("follower threshold beta in share memory not found or version is zero for {}", base_asset);
                }
                continue;
            }

            if (benchmark_beta_threshold->version_number < benchmark_beta_version
                || follower_beta_threshold->version_number < follower_beta_version
                || early_run_threshold->version_number < early_run_version) {

                if (benchmark_beta_threshold->version_number > benchmark_beta_version) {
                    benchmark_beta_version = benchmark_beta_threshold->version_number;
                }
                if (follower_beta_threshold->version_number > follower_beta_version) {
                    follower_beta_version = follower_beta_threshold->version_number;
                }
                if ((*early_run_threshold).version_number > early_run_version) {
                    early_run_version = (*early_run_threshold).version_number;
                }
                if (rand_log_number < 10) {
                    warn_log("threshold version is old in share memory for {}", base_asset);
                }
                continue;
            }

            benchmark_beta_version = benchmark_beta_threshold->version_number;
            follower_beta_version = follower_beta_threshold->version_number;
            early_run_version = (*early_run_threshold).version_number;

            if (now > (*benchmark_ticker).update_time + config.benchmark_ticker_validity_millis ||
                now > (*follower_ticker).update_time + config.follower_ticker_validity_millis) {
                if (rand_log_number < 1) {
                    // warn_log("ticker timestamp expired for {}", base_asset);
                }
                continue;
            }
            if (now >= follower_ticker->update_time) {
                ticker_delay_millis = int(now - follower_ticker->update_time);
            }

            if (now > (*benchmark_beta_threshold).time_mills + config.threshold_validity_millis ||
                now > (*follower_beta_threshold).time_mills + config.threshold_validity_millis ||
                now > (*early_run_threshold).time_mills + config.threshold_validity_millis) {
                if (rand_log_number < 10) {
                    warn_log("threshold timestamp expired for {}", base_asset);
                }
                continue;
            }

            optional<AccountPositionInfo> position = context.get_balance_position_composite().copy_position(follower_inst_id);
            if (!position.has_value()) {
                warn_log("position not found for {}", follower_inst_id);
                continue;
            }

            double position_reduce_ratio = 0;
            if (config.enable_position_threshold) {
                optional<PositionThresholdInfo> position_threshold = context.get_balance_position_composite().copy_position_threshold(follower_inst_id);
                if (position_threshold.has_value()) {
                    position_reduce_ratio = position_threshold.value().positionReduceRatio;
                }
            }

            // dynamic adjust threshold with position amount
            bool stop_buy, stop_sell = false;
            if ((*position).positionAmountAbs >= inst_config.max_position * config.max_position_zoom) {
                if ((*position).positionSide == binance::PositionSide_LONG) {
                    stop_buy = true;
                } else {
                    stop_sell = true;
                }
            }

            if (config.enable_write_parameter_log) {
                info_log("check buy order parameters: base_asset={}, benchmark_ticker_bid={}, follower_ticker_ask={}, bid_beta_threshold={}, bid_ask_median={}, ask_beta_threshold={}, beta={}, benchmark_bid_size={}, min_ticker_notional={}, follower_ask_size={} position_reduce_ratio={} {}>={}?{} {} {}",
                    base_asset,
                    (*benchmark_ticker).bid_price,
                    (*follower_ticker).ask_price,
                    (*benchmark_beta_threshold).bid_beta_threshold,
                    (*early_run_threshold).bid_ask_median,
                    (*follower_beta_threshold).ask_beta_threshold,
                    inst_config.beta,
                    (*benchmark_ticker).bid_size,
                    inst_config.min_ticker_notional,
                    (*follower_ticker).ask_size,
                    position_reduce_ratio,
                    (benchmark_ticker->bid_price/(1 + (benchmark_beta_threshold->bid_beta_threshold + follower_beta_threshold->ask_beta_threshold) * (1 + position_reduce_ratio))),
                    ((follower_ticker->ask_price+early_run_threshold->bid_ask_median)),
                    (benchmark_ticker->bid_price/(1 + (benchmark_beta_threshold->bid_beta_threshold + follower_beta_threshold->ask_beta_threshold) * (1 + position_reduce_ratio))) 
                        >= ((follower_ticker->ask_price+early_run_threshold->bid_ask_median)),
                    benchmark_ticker->bid_price * benchmark_ticker->bid_size >= inst_config.min_ticker_notional,
                    benchmark_ticker->bid_price * benchmark_ticker->bid_size >= follower_ticker->ask_price * follower_ticker->ask_size * inst_config.min_ticker_notional_multiple
                );

                info_log("check sell order parameters: base_asset={} benchmark_ticker_ask={}, follower_ticker_bid={}, ask_beta_threshold={}, ask_bid_median={}, bid_beta_threshold={}, beta={}, benchmark_ask_size={}, min_ticker_notional={}, follower_bid_size={} position_reduce_ratio={} {}<={}?{} {} {}",
                    base_asset,
                    (*benchmark_ticker).ask_price,
                    (*follower_ticker).bid_price,
                    benchmark_beta_threshold->ask_beta_threshold,
                    (*early_run_threshold).ask_bid_median,
                    follower_beta_threshold->bid_beta_threshold,
                    inst_config.beta,
                    (*benchmark_ticker).ask_size,
                    inst_config.min_ticker_notional,
                    (*follower_ticker).bid_size,
                    position_reduce_ratio,
                    (benchmark_ticker->ask_price*(1 + (benchmark_beta_threshold->ask_beta_threshold + follower_beta_threshold->bid_beta_threshold) * (1 - position_reduce_ratio))),
                    ((follower_ticker->bid_price + early_run_threshold->ask_bid_median)),
                    (benchmark_ticker->ask_price*(1 + (benchmark_beta_threshold->ask_beta_threshold + follower_beta_threshold->bid_beta_threshold) * (1 - position_reduce_ratio)))
                        <= ((follower_ticker->bid_price + early_run_threshold->ask_bid_median)),
                   benchmark_ticker->ask_price * benchmark_ticker->ask_size >= inst_config.min_ticker_notional,
                   benchmark_ticker->ask_price * benchmark_ticker->ask_size >= follower_ticker->bid_price * follower_ticker->bid_size * inst_config.min_ticker_notional_multiple
                );
            }
            
            if ((benchmark_ticker->bid_price/(1 + (benchmark_beta_threshold->bid_beta_threshold + follower_beta_threshold->ask_beta_threshold) * (1 + position_reduce_ratio))) 
                    >= ((follower_ticker->ask_price+early_run_threshold->bid_ask_median))
                && benchmark_ticker->bid_price * benchmark_ticker->bid_size >= inst_config.min_ticker_notional
                && benchmark_ticker->bid_price * benchmark_ticker->bid_size >= follower_ticker->ask_price * follower_ticker->ask_size * inst_config.min_ticker_notional_multiple
            ) {

                // make buy-side order
                double order_size = inst_config.order_size * config.order_size_zoom;
                bool position_close = false;
                bool price_is_adjusted = false;
                int reduce_only = 0;
                if ((*position).positionSide == binance::PositionSide_SHORT) {
                    position_close = true;
                    if (config.enable_order_reduce_only && order_size > (*position).positionAmountAbs) {
                        reduce_only = 1;
                        order_size = (*position).positionAmountAbs;
                    }
                }

                double original_buy_price = follower_ticker->ask_price * (1 + config.order_price_margin);
                original_buy_price = decimal_process(original_buy_price, follower_exchange_info.pricePrecision);
				double adjusted_buy_price = original_buy_price;
                // make sure buy price not higher than the threshold when give little profit
                if (config.order_price_margin > 0 &&
                    adjusted_buy_price > benchmark_ticker->bid_price / (1 + inst_config.beta) - early_run_threshold->bid_ask_median) {
					
                    adjusted_buy_price = benchmark_ticker->bid_price / (1 + inst_config.beta) - early_run_threshold->bid_ask_median;
                    adjusted_buy_price = decimal_process(adjusted_buy_price, follower_exchange_info.pricePrecision);
                    price_is_adjusted = true;
				}

                shm_mng::OrderShm order_buy;
                strcpy(order_buy.inst_id, follower_inst_id.c_str());
                strcpy(order_buy.type, binance::ORDER_TYPE_LIMIT.c_str());
                strcpy(order_buy.side, binance::ORDER_SIDE_BUY.c_str());
                // strcpy(order_buy.pos_side, binance::PositionSide_LONG.c_str());
                strcpy(order_buy.pos_side, binance::PositionSide_BOTH.c_str());
                strcpy(order_buy.time_in_force, binance::TimeInForce_IOC.c_str());
                order_buy.price = adjusted_buy_price;
                order_buy.volume = order_size;
                order_buy.reduce_only = reduce_only;
                std::string client_order_id = gen_client_order_id(true, price_is_adjusted, ticker_delay_millis, position_close);
                strcpy(order_buy.client_order_id, client_order_id.c_str());
                order_buy.update_time = now;

                bool same_make_order = false;
                if (adjusted_buy_price == latest_buy_order_price && latest_buy_order_millis + config.same_price_pause_time_millis > now) {
                    // price is same with latest, should reduce order making ratio
                    if (reduce_only == 1) {
                        // close position
                        long small_pause_time_millis = std::max(int(config.same_price_pause_time_millis * 90 / 100), 50);
                        same_make_order = latest_buy_order_millis + small_pause_time_millis <= now;
                    }
                } else {
                    same_make_order = true;
                }

                if (same_make_order) {
                    latest_buy_order_price = adjusted_buy_price;
                    latest_buy_order_millis = now;
                }

                int shm_updated = 0;
                bool config_make_order = context.dynamic_could_make_order();
                bool config_make_open_position_order = context.dynamic_could_make_open_position_order();
                bool should_write_log = false;
                if (!stop_buy && config_make_order && same_make_order) {
                    should_write_log = true;
                    if (position_close || config_make_open_position_order) {
                        shm_updated = shm_mng::order_shm_writer_update(context.get_shm_store_info().order_start, order_shm_index, order_buy);
                    }
                } else {
                    should_write_log = rand_order_log_number < 1;
                }

                float price_adjusted_ratio = adjusted_buy_price / original_buy_price;
                if (should_write_log) {
                    // reduce log frequency
                    info_log("update buy order: config_make_order={} config_make_open_position_order={} position_close={} same_make_order={} stop_buy={} shm_updated={} inst_id={} price={}/{}/{} size={} client_id={} ticker_version={}/{} threshold_version={}/{}/{}",
                        config_make_order, config_make_open_position_order, position_close, same_make_order, stop_buy, shm_updated, follower_inst_id, original_buy_price, order_buy.price,
                        price_adjusted_ratio, order_buy.volume, client_order_id, benchmark_ticker_version, follower_ticker_version, benchmark_beta_version, follower_beta_version, early_run_version);
                }
            } else if ((benchmark_ticker->ask_price*(1 + (benchmark_beta_threshold->ask_beta_threshold + follower_beta_threshold->bid_beta_threshold) * (1 - position_reduce_ratio)))
                    <= ((follower_ticker->bid_price + early_run_threshold->ask_bid_median))
                && benchmark_ticker->ask_price * benchmark_ticker->ask_size >= inst_config.min_ticker_notional
                && benchmark_ticker->ask_price * benchmark_ticker->ask_size >= follower_ticker->bid_price * follower_ticker->bid_size * inst_config.min_ticker_notional_multiple
            ) {

                // make sell-side order
                double order_size = inst_config.order_size * config.order_size_zoom;
                bool position_close = false;
                bool price_is_adjusted = false;
                int reduce_only = 0;
                if ((*position).positionSide == binance::PositionSide_LONG) {
                    position_close = true;
                    if (config.enable_order_reduce_only && order_size > (*position).positionAmountAbs) {
                        order_size = (*position).positionAmountAbs;
                        reduce_only = 1;
                    }
                }

                double original_sell_price = follower_ticker->bid_price * (1 - config.order_price_margin);
                original_sell_price = decimal_process(original_sell_price, follower_exchange_info.pricePrecision);
                double adjusted_sell_price = original_sell_price;
				// make sure sell price not lower than the threshold when give little profit
                if (config.order_price_margin > 0 &&
                    adjusted_sell_price < benchmark_ticker->ask_price * (1 + inst_config.beta) - early_run_threshold->ask_bid_median) {
					
                    adjusted_sell_price = benchmark_ticker->ask_price * (1 + inst_config.beta) - early_run_threshold->ask_bid_median;
                    adjusted_sell_price = decimal_process(adjusted_sell_price, follower_exchange_info.pricePrecision);
                    price_is_adjusted = true;
				}

                shm_mng::OrderShm order_sell;
                strcpy(order_sell.inst_id, follower_inst_id.c_str());
                strcpy(order_sell.type, binance::ORDER_TYPE_LIMIT.c_str());
                strcpy(order_sell.side, binance::ORDER_SIDE_SELL.c_str());
                // strcpy(order_buy.pos_side, binance::PositionSide_SHORT.c_str());
                strcpy(order_sell.pos_side, binance::PositionSide_BOTH.c_str());
                strcpy(order_sell.time_in_force, binance::TimeInForce_IOC.c_str());
                order_sell.price = adjusted_sell_price;
                order_sell.volume = order_size;
                order_sell.reduce_only = reduce_only;
                std::string client_order_id = gen_client_order_id(false, price_is_adjusted, ticker_delay_millis, position_close);
                strcpy(order_sell.client_order_id, client_order_id.c_str());
                order_sell.update_time = now;
                
                bool same_make_order = false;
                if (adjusted_sell_price == latest_sell_order_price && latest_sell_order_millis + config.same_price_pause_time_millis > now) {
                    // price is same with latest, should reduce order making ratio
                    if (reduce_only == 1) {
                        // close position
                        long small_pause_time_millis = std::max(int(config.same_price_pause_time_millis * 90 / 100), 50);
                        same_make_order = latest_sell_order_millis + small_pause_time_millis <= now;
                    }
                } else {
                    same_make_order = true;
                }

                if (same_make_order) {
                    latest_sell_order_price = adjusted_sell_price;
                    latest_sell_order_millis = now;
                }

                int shm_updated = 0;
                bool config_make_order = context.dynamic_could_make_order();
                bool config_make_open_position_order = context.dynamic_could_make_open_position_order();
                bool should_write_log = false;
                if (!stop_sell && config_make_order && same_make_order) {
                    should_write_log = true;
                    if (position_close || config_make_open_position_order) {
                        shm_updated = shm_mng::order_shm_writer_update(context.get_shm_store_info().order_start, order_shm_index, order_sell);
                    }
                } else {
                    // reduce log frequency
                    should_write_log = rand_order_log_number < 1;
                }

                float price_adjusted_ratio = adjusted_sell_price / original_sell_price;
                if (should_write_log) {
                    info_log("update sell order: config_make_order={} config_make_open_position_order={} position_close={} same_make_order={} stop_sell={} shm_updated={} inst_id={} price={}/{}/{} size={} client_id={} ticker_version={}/{} threshold_version={}/{}/{}",
                        config_make_order, config_make_open_position_order, position_close, same_make_order, stop_sell, shm_updated, follower_inst_id, original_sell_price, order_sell.price, price_adjusted_ratio, order_sell.volume,
                        client_order_id, benchmark_ticker_version, follower_ticker_version, benchmark_beta_version, follower_beta_version, early_run_version);
                }
            }
        }
    }
}