#include "strategy_processor.h"

#include <cstdlib> // For exit()

namespace actuary {

    void start_strategy_process(ActuaryConfig& config, GlobalContext& context) {}

    void check_signal_make_order(ActuaryConfig& config, GlobalContext& context, std::string& base_asset) {

        int benchmark_shm_index = -1;
        int follower_shm_index = -1;
        int threshold_shm_index = -1;
        int order_shm_index = -1;

        auto benchmark = context.get_shm_benchmark_ticker_mapping().find(base_asset);
        if (benchmark != context.get_shm_benchmark_ticker_mapping().end()) {
            benchmark_shm_index = (*benchmark).second;
        }

        auto follower = context.get_shm_follower_ticker_mapping().find(base_asset);
        if (follower != context.get_shm_follower_ticker_mapping().end()) {
            follower_shm_index = (*follower).second;
        }

        auto threshold = context.get_shm_threshold_mapping().find(base_asset);
        if (threshold != context.get_shm_threshold_mapping().end()) {
            threshold_shm_index = (*threshold).second;
        }

        auto order = context.get_shm_order_mapping().find(base_asset);
        if (order != context.get_shm_order_mapping().end()) {
            order_shm_index = (*order).second;
        }

        info_log("find shm mapping index for {}: benchmark_shm_index={} follower_shm_index={} threshold_shm_index={} order_shm_index={}",
            base_asset, benchmark_shm_index, follower_shm_index, threshold_shm_index, order_shm_index
        );
        if (benchmark_shm_index < 0 || follower_shm_index < 0 || threshold_shm_index < 0 || order_shm_index < 0) {
            err_log("fail to find shm mapping index");
            exit(-1);
        }

        auto inst_config_auto = context.get_inst_config().inst_map.find(base_asset);
        if (inst_config_auto == context.get_inst_config().inst_map.end()) {
            warn_log("inst config not found for {}", base_asset);
            exit(-1);
        }
        InstConfigItem inst_config = (*inst_config_auto).second;

        long benchmark_ticker_version, follower_ticker_version, earyly_run_version, beta_version = 0;
        while (true) {
            std::shared_ptr<shm_mng::TickerInfoShm> benchmark_ticker = shm_mng::ticker_shm_reader_get(context.get_shm_store_info().benchmark_start, benchmark_shm_index);
            std::shared_ptr<shm_mng::TickerInfoShm> follower_ticker = shm_mng::ticker_shm_reader_get(context.get_shm_store_info().follower_start, follower_shm_index);
            if (benchmark_ticker == nullptr || follower_ticker == nullptr ||
                (*benchmark_ticker).version_number == 0 || (*follower_ticker).version_number == 0) {
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
                continue;
            }
            if ((*beta_threshold).version_number <= beta_version && (*early_run_threshold).version_number <= earyly_run_version) {
                beta_version = (*beta_threshold).version_number;
                earyly_run_version = (*early_run_threshold).version_number;
                continue;
            }

            beta_version = (*beta_threshold).version_number;
            earyly_run_version = (*early_run_threshold).version_number;

            if ((*benchmark_ticker).bid_price > ((*follower_ticker).ask_price + (*early_run_threshold).bid_ask_median) * (1 + (*beta_threshold).volatility_multiplier * inst_config.beta) && (*benchmark_ticker).bid_size > inst_config.max_ticker_size && (*follower_ticker).ask_size < inst_config.min_ticker_size) {
                // TODO make buy-side order

            } else if ((*benchmark_ticker).ask_price < ((*follower_ticker).bid_price + (*early_run_threshold).ask_bid_median) / (1 + (*beta_threshold).volatility_multiplier * inst_config.beta) && (*benchmark_ticker).ask_size > inst_config.max_ticker_size && (*follower_ticker).bid_size < inst_config.min_ticker_size) {
                // TODO make sell-side order

            }
        }
    }
}