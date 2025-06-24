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
        
        while (true) {

            if (config.loop_pause_time_millis > 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(config.loop_pause_time_millis));
            }
            int rand_log_number = rand_log.randInt();
            int rand_order_log_number = rand_order_log.randInt();
            uint64_t now = binance::get_current_ms_epoch();

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

            if ((*benchmark_ticker).version_number == benchmark_ticker_version && (*follower_ticker).version_number == follower_ticker_version ||
                (*benchmark_ticker).version_number < benchmark_ticker_version || (*follower_ticker).version_number < follower_ticker_version) {
                
                if ((*benchmark_ticker).version_number > benchmark_ticker_version) {
                    benchmark_ticker_version = (*benchmark_ticker).version_number;
                }
                if ((*follower_ticker).version_number > follower_ticker_version) {
                    follower_ticker_version = (*follower_ticker).version_number;
                }
                if (rand_log_number < 10) {
                    warn_log("ticker version is old in share memory for {}", base_asset);
                }
                continue;
            }

            benchmark_ticker_version = (*benchmark_ticker).version_number;
            follower_ticker_version = (*follower_ticker).version_number;

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

            if (now > (*benchmark_ticker).update_time + config.ticker_validity_millis ||
                now > (*follower_ticker).update_time + config.ticker_validity_millis) {
                if (rand_log_number < 10) {
                    warn_log("ticker timestamp expired for {}", base_asset);
                }
                continue;
            }
            if (now > (*benchmark_beta_threshold).time_mills + config.threshold_validity_millis ||
                now > (*follower_beta_threshold).time_mills + config.threshold_validity_millis ||
                now > (*early_run_threshold).time_mills + config.threshold_validity_millis) {
                if (rand_log_number < 10) {
                    warn_log("threshold timestamp expired for {}", base_asset);
                }
                continue;
            }

            // info_log("parameters: {} {}, {}, {}, {}, {}, {}, {}, {}, {}",
            //     base_asset,
            //     (*benchmark_ticker).bid_price,
            //     (*follower_ticker).ask_price,
            //     (*early_run_threshold).bid_ask_median,
            //     (*follower_beta_threshold).volatility_multiplier,
            //     inst_config.beta,
            //     (*benchmark_ticker).bid_size,
            //     inst_config.max_ticker_size,
            //     (*follower_ticker).ask_size,
            //     inst_config.min_ticker_size
            // );

            // info_log("parameters: {} {}, {}, {}, {}, {}, {}, {}, {}, {}",
            //     base_asset,
            //     (*benchmark_ticker).ask_price,
            //     (*follower_ticker).bid_price,
            //     (*early_run_threshold).ask_bid_median,
            //     (*follower_beta_threshold).volatility_multiplier,
            //     inst_config.beta,
            //     (*benchmark_ticker).ask_size,
            //     inst_config.max_ticker_size,
            //     (*follower_ticker).bid_size,
            //     inst_config.min_ticker_size
            // );

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
            if ((*position).positionAmt >= inst_config.max_position * config.max_position_zoom) {
                if ((*position).positionSide == binance::PositionSide_LONG) {
                    stop_buy = true;
                } else {
                    stop_sell = true;
                }
            }

            /*
            if (bnTicker.BidPrice/(1+benchmarkThreshold.BidBetaThreshold*(1+positionReduceRatio)) >=
				(okxTicker.AskPrice+earlyRunThreshold.BnBidOkxAskPriceDiffMedian)*(1+instdBetaThreshold.AskBetaThreshold*(1+positionReduceRatio))) &&
				bnTicker.BidVolume > instConfig.MinTickerSize*instConfig.ContractValue && okxTicker.AskVolume < instConfig.MaxTickerSize
            */
            
            if ((benchmark_ticker->bid_price/(1 + benchmark_beta_threshold->bid_beta_threshold * (1 + position_reduce_ratio))) 
                >= ((follower_ticker->ask_price+early_run_threshold->bid_ask_median) * (1 + follower_beta_threshold->ask_beta_threshold * (1 + position_reduce_ratio)))
                && (*benchmark_ticker).bid_size > inst_config.min_ticker_size
                && (*follower_ticker).ask_size < inst_config.max_ticker_size
            ) {
                // make buy-side order
                double order_size = inst_config.order_size * config.order_size_zoom;
                bool position_close = false;
                int reduce_only = 0;
                if ((*position).positionSide == binance::PositionSide_SHORT
                    && order_size > (*position).positionAmt) {
                    position_close = true;
                    if (config.enable_order_reduce_only) {
                        reduce_only = 1;
                        order_size = (*position).positionAmt;
                    }
                }

                double buy_price = follower_ticker->ask_price * (1 + config.order_price_margin);
				// make sure buy price not higher than the threshold
                if (buy_price > benchmark_ticker->bid_price / (1 + inst_config.beta) - early_run_threshold->bid_ask_median) {
					buy_price = benchmark_ticker->bid_price / (1 + inst_config.beta) - early_run_threshold->bid_ask_median;
				}
                buy_price = decimal_process(buy_price, follower_exchange_info.pricePrecision);

                shm_mng::OrderShm order_buy;
                strcpy(order_buy.inst_id, follower_inst_id.c_str());
                strcpy(order_buy.type, binance::ORDER_TYPE_LIMIT.c_str());
                strcpy(order_buy.side, binance::ORDER_SIDE_BUY.c_str());
                // strcpy(order_buy.pos_side, binance::PositionSide_LONG.c_str());
                strcpy(order_buy.pos_side, binance::PositionSide_BOTH.c_str());
                strcpy(order_buy.time_in_force, binance::TimeInForce_IOC.c_str());
                order_buy.price = buy_price;
                order_buy.volume = order_size;
                order_buy.reduce_only = reduce_only;
                std::string client_order_id = gen_client_order_id(true);
                strcpy(order_buy.client_order_id, client_order_id.c_str());
                order_buy.update_time = now;

                bool same_make_order = false;
                if (buy_price == latest_buy_order_price && latest_buy_order_millis + config.same_price_pause_time_millis > now) {
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
                    latest_buy_order_price = buy_price;
                    latest_buy_order_millis = now;
                }

                int shm_updated = 0;
                bool config_make_order = context.dynamic_could_make_order();
                bool should_write_log = false;
                if (!stop_buy && config_make_order && same_make_order) {
                    should_write_log = true;
                    shm_updated = shm_mng::order_shm_writer_update(context.get_shm_store_info().order_start, order_shm_index, order_buy);
                } else {
                    should_write_log = rand_order_log_number < 1;
                }

                if (should_write_log) {
                    // reduce log frequency
                    info_log("update buy order: config_make_order={} same_make_order={} stop_buy={} shm_updated={} inst_id={} price={} size={} client_id={} position_close={} ticker_version={}/{} threshold_version={}/{}/{}",
                        config_make_order, same_make_order, stop_buy, shm_updated, follower_inst_id, order_buy.price, order_buy.volume, client_order_id, position_close,
                        benchmark_ticker_version, follower_ticker_version, benchmark_beta_version, follower_beta_version, early_run_version);
                }
            }

            /*
             (bnTicker.AskPrice*(1+benchmarkThreshold.AskBetaThreshold*(1-positionReduceRatio)) <=
				(okxTicker.BidPrice+earlyRunThreshold.BnAskOkxBidPriceDiffMedian)/(1+instdBetaThreshold.BidBetaThreshold*(1-positionReduceRatio))) &&
				bnTicker.AskVolume > instConfig.MinTickerSize*instConfig.ContractValue && okxTicker.BidVolume < instConfig.MaxTickerSize
            */
            if ((benchmark_ticker->ask_price*(1 + benchmark_beta_threshold->ask_beta_threshold * (1 - position_reduce_ratio))) 
                <= ((follower_ticker->bid_price + early_run_threshold->ask_bid_median) / (1 + follower_beta_threshold->bid_beta_threshold * (1 - position_reduce_ratio)))
                && benchmark_ticker->ask_size > inst_config.min_ticker_size
                && follower_ticker->bid_size < inst_config.max_ticker_size
            ) {
                // make sell-side order
                double order_size = inst_config.order_size * config.order_size_zoom;
                bool position_close = false;
                int reduce_only = 0;
                if ((*position).positionSide == binance::PositionSide_LONG
                    && order_size > (*position).positionAmt) {
                    position_close = true;
                    if (config.enable_order_reduce_only) {
                        order_size = (*position).positionAmt;
                        reduce_only = 1;
                    }
                }

                double sell_price = follower_ticker->bid_price * (1 - config.order_price_margin);
				// make sure sell price not lower than the threshold
                if (sell_price < benchmark_ticker->ask_price * (1 + inst_config.beta) - early_run_threshold->ask_bid_median) {
					sell_price = benchmark_ticker->ask_price * (1 + inst_config.beta) - early_run_threshold->ask_bid_median;
				}
                sell_price = decimal_process(sell_price, follower_exchange_info.pricePrecision);

                shm_mng::OrderShm order_sell;
                strcpy(order_sell.inst_id, follower_inst_id.c_str());
                strcpy(order_sell.type, binance::ORDER_TYPE_LIMIT.c_str());
                strcpy(order_sell.side, binance::ORDER_SIDE_SELL.c_str());
                // strcpy(order_buy.pos_side, binance::PositionSide_SHORT.c_str());
                strcpy(order_sell.pos_side, binance::PositionSide_BOTH.c_str());
                strcpy(order_sell.time_in_force, binance::TimeInForce_IOC.c_str());
                order_sell.price = sell_price;
                order_sell.volume = order_size;
                order_sell.reduce_only = reduce_only;
                std::string client_order_id = gen_client_order_id(false);
                strcpy(order_sell.client_order_id, client_order_id.c_str());
                order_sell.update_time = now;
                
                bool same_make_order = false;
                if (sell_price == latest_sell_order_price && latest_sell_order_millis + config.same_price_pause_time_millis > now) {
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
                    latest_sell_order_price = sell_price;
                    latest_sell_order_millis = now;
                }

                int shm_updated = 0;
                bool config_make_order = context.dynamic_could_make_order();
                bool should_write_log = false;
                if (!stop_sell && config_make_order && same_make_order) {
                    should_write_log = true;
                    shm_updated = shm_mng::order_shm_writer_update(context.get_shm_store_info().order_start, order_shm_index, order_sell);
                } else {
                    // reduce log frequency
                    should_write_log = rand_order_log_number < 1;
                }

                if (should_write_log) {
                    info_log("update sell order: config_make_order={} same_make_order={} stop_sell={} shm_updated={} inst_id={} price={} size={} client_id={} position_close={} ticker_version={}/{} threshold_version={}/{}/{}",
                        config_make_order, same_make_order, stop_sell, shm_updated, follower_inst_id, order_sell.price, order_sell.volume, client_order_id, position_close,
                        benchmark_ticker_version, follower_ticker_version, benchmark_beta_version, follower_beta_version, early_run_version);
                }
            }
        }
    }
}