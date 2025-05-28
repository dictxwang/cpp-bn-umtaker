#include "strategy_processor.h"

#include <cstdlib> // For exit()
#include <thread>

namespace actuary {

    void start_strategy_processors(ActuaryConfig& config, GlobalContext& context) {

        for (size_t i = 0; i < config.base_asset_list.size(); ++i) {

            std::string base_asset = config.base_asset_list[i];
            std::thread check_task_thread(check_signal_make_order, std::ref(config), std::ref(context), base_asset);
            check_task_thread.detach();
            info_log("start strategy processor for {}", base_asset);
        }
    }

    void check_signal_make_order(ActuaryConfig& config, GlobalContext& context, std::string base_asset) {

        std::string benchmark_inst_id = base_asset + config.benchmark_quote_asset;
        std::string follower_inst_id = base_asset + config.follower_quote_asset;

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
            exit(-1);
        }

        auto inst_config_auto = context.get_inst_config().inst_map.find(base_asset);
        if (inst_config_auto == context.get_inst_config().inst_map.end()) {
            warn_log("inst config not found for {}", base_asset);
            exit(-1);
        }

        RandomIntGen rand;
        rand.init(0, 10000);
        InstConfigItem inst_config = (*inst_config_auto).second;
        long benchmark_ticker_version, follower_ticker_version, early_run_version, beta_version = 0;
        while (true) {

            // TODO delete it
            std::this_thread::sleep_for(std::chrono::seconds(5));
            int rnd_number = rand.randInt();
            uint64_t now = binance::get_current_ms_epoch();

            std::shared_ptr<shm_mng::TickerInfoShm> benchmark_ticker = shm_mng::ticker_shm_reader_get(context.get_shm_store_info().benchmark_start, benchmark_shm_index);
            std::shared_ptr<shm_mng::TickerInfoShm> follower_ticker = shm_mng::ticker_shm_reader_get(context.get_shm_store_info().follower_start, follower_shm_index);
            if (benchmark_ticker == nullptr || follower_ticker == nullptr) {
                if (rnd_number < 10) {
                    warn_log("ticker in share memory not found for {}", base_asset);
                }
                continue;
            }
            if ((*benchmark_ticker).version_number == 0 || (*follower_ticker).version_number == 0) {
                if (rnd_number < 10) {
                    warn_log("ticker version in share memory are zero {}", base_asset);
                }
                continue;
            }

            if ((*benchmark_ticker).version_number <= benchmark_ticker_version && (*follower_ticker).version_number <= follower_ticker_version) {
                benchmark_ticker_version = (*benchmark_ticker).version_number;
                follower_ticker_version = (*follower_ticker).version_number;
                continue;
            }

            benchmark_ticker_version = (*benchmark_ticker).version_number;
            follower_ticker_version = (*follower_ticker).version_number;

            std::shared_ptr<shm_mng::EarlyRunThresholdShm> early_run_threshold = shm_mng::early_run_shm_reader_get(context.get_shm_store_info().early_run_start, threshold_shm_index);
            std::shared_ptr<shm_mng::BetaThresholdShm> beta_threshold = shm_mng::beta_shm_reader_get(context.get_shm_store_info().beta_start, threshold_shm_index);
            
            if (beta_threshold == nullptr || (*beta_threshold).version_number == 0 ||
                early_run_threshold == nullptr || (*early_run_threshold).version_number == 0) {
                if (rnd_number < 10) {
                    warn_log("threshold in share memory not found or version is zero for {}", base_asset);
                }
                continue;
            }
            if ((*beta_threshold).version_number <= beta_version && (*early_run_threshold).version_number <= early_run_version) {
                beta_version = (*beta_threshold).version_number;
                early_run_version = (*early_run_threshold).version_number;
                continue;
            }

            beta_version = (*beta_threshold).version_number;
            early_run_version = (*early_run_threshold).version_number;

            if (now > (*benchmark_ticker).update_time + config.ticker_valid_millis ||
                now > (*follower_ticker).update_time + config.ticker_valid_millis ||
                now > (*beta_threshold).time_mills + config.ticker_valid_millis ||
                now > (*early_run_threshold).time_mills + config.ticker_valid_millis) {
                if (rnd_number < 10) {
                    warn_log("timestamp expired for {}", base_asset);
                }
                continue;
            }

            // info_log("parameters: {} {}, {}, {}, {}, {}, {}",
            //     base_asset,
            //     (*benchmark_ticker).bid_price,
            //     ((*follower_ticker).ask_price + (*early_run_threshold).bid_ask_median) * (1 + (*beta_threshold).volatility_multiplier * inst_config.beta),
            //     (*benchmark_ticker).bid_size,
            //     inst_config.max_ticker_size,
            //     (*follower_ticker).ask_size,
            //     inst_config.min_ticker_size
            // );

            // info_log("parameters: {} {}, {}, {}, {}, {}, {}",
            //     base_asset,
            //     (*benchmark_ticker).ask_price,
            //     ((*follower_ticker).bid_price + (*early_run_threshold).ask_bid_median) / (1 + (*beta_threshold).volatility_multiplier * inst_config.beta),
            //     (*benchmark_ticker).ask_size,
            //     inst_config.max_ticker_size,
            //     (*follower_ticker).bid_size,
            //     inst_config.min_ticker_size
            // );

            // TODO delete true condition
            if (true || (*benchmark_ticker).bid_price > ((*follower_ticker).ask_price + (*early_run_threshold).bid_ask_median) * (1 + (*beta_threshold).volatility_multiplier * inst_config.beta) && (*benchmark_ticker).bid_size > inst_config.max_ticker_size && (*follower_ticker).ask_size < inst_config.min_ticker_size) {
                // make buy-side order
                shm_mng::OrderShm order_buy;
                strcpy(order_buy.inst_id, follower_inst_id.c_str());
                strcpy(order_buy.type, binance::ORDER_TYPE_LIMIT.c_str());
                strcpy(order_buy.side, binance::ORDER_SIDE_BUY.c_str());
                // strcpy(order_buy.pos_side, binance::PositionSide_LONG.c_str());
                strcpy(order_buy.time_in_force, binance::TimeInForce_IOC.c_str());
                order_buy.price = (*benchmark_ticker).ask_price;
                order_buy.volume = inst_config.order_size;
                std::string client_order_id = gen_client_order_id(true);
                strcpy(order_buy.client_order_id, client_order_id.c_str());

                int updated = shm_mng::order_shm_writer_update(context.get_shm_store_info().order_start, order_shm_index, order_buy);
                // TODO reduce too many logs
                info_log("update buy order: inst_id={} price={} size={} client_id={} updated={} ticker_version={}/{} threshold_version={}/{}",
                    follower_inst_id, order_buy.price, order_buy.volume, client_order_id, updated,
                    benchmark_ticker_version, follower_ticker_version, beta_version, early_run_version);
            
            } else if ((*benchmark_ticker).ask_price < ((*follower_ticker).bid_price + (*early_run_threshold).ask_bid_median) / (1 + (*beta_threshold).volatility_multiplier * inst_config.beta) && (*benchmark_ticker).ask_size > inst_config.max_ticker_size && (*follower_ticker).bid_size < inst_config.min_ticker_size) {
                // make sell-side order
                shm_mng::OrderShm order_sell;
                strcpy(order_sell.inst_id, follower_inst_id.c_str());
                strcpy(order_sell.type, binance::ORDER_TYPE_LIMIT.c_str());
                strcpy(order_sell.side, binance::ORDER_SIDE_SELL.c_str());
                // strcpy(order_buy.pos_side, binance::PositionSide_SHORT.c_str());
                strcpy(order_sell.time_in_force, binance::TimeInForce_IOC.c_str());
                order_sell.price = (*benchmark_ticker).bid_price;
                order_sell.volume = inst_config.order_size;
                std::string client_order_id = gen_client_order_id(false);
                strcpy(order_sell.client_order_id, client_order_id.c_str());
                
                int updated = shm_mng::order_shm_writer_update(context.get_shm_store_info().order_start, order_shm_index, order_sell);
                // TODO reduce too many logs
                info_log("update sell order: inst_id={} price={} size={} client_id={} updated={} ticker_version={}/{} threshold_version={}/{}",
                    follower_inst_id, order_sell.price, order_sell.volume, client_order_id, updated,
                    benchmark_ticker_version, follower_ticker_version, beta_version, early_run_version);
            }
        }
    }
}