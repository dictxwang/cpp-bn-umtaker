#include "global_context.h"

namespace receiver {
    
    void GlobalContext::init(ReceiverConfig& config) {
        this->benchmark_ticker_composite.init(config.benchmark_quote_asset, config.base_asset_list, config.calculate_sma_interval_seconds);
        this->follower_ticker_composite.init(config.follower_quote_asset, config.base_asset_list, config.calculate_sma_interval_seconds);

        this->price_offset_composite.init(config.base_asset_list, config.calculate_sma_interval_seconds);
        this->early_run_threshold_composite.init(config.base_asset_list);
        this->beta_threshold_composite.init(config.base_asset_list);

        for (string base : config.base_asset_list) {
            string benchmark_inst = base + config.benchmark_quote_asset;
            string follower_inst = base + config.follower_quote_asset;
            this->benchmark_inst_ids.push_back(benchmark_inst);
            this->follower_inst_ids.push_back(follower_inst);
            this->inst_ids_set.insert(benchmark_inst);
            this->inst_ids_set.insert(follower_inst);
        }

        this->inst_config.loadInstConfig(config.inst_config_file);
    };

    InstConfig& GlobalContext::get_inst_config() {
        return this->inst_config;
    }

    EarlyRunThresholdComposite& GlobalContext::get_early_run_threshold_composite() {
        return this->early_run_threshold_composite;
    }

    BetaThresholdComposite& GlobalContext::get_beta_threshold_composite() {
        return this->beta_threshold_composite;
    }

    TickerComposite& GlobalContext::get_benchmark_ticker_composite() {
        return this->benchmark_ticker_composite;
    }

    TickerComposite& GlobalContext::get_follower_ticker_composite() {
        return this->follower_ticker_composite;
    }

    PriceOffsetComposite& GlobalContext::get_price_offset_composite() {
        return this->price_offset_composite;
    }

    vector<string>& GlobalContext::get_benchmark_inst_ids() {
        return this->benchmark_inst_ids;
    }

    vector<string>& GlobalContext::get_follower_inst_ids() {
        return this->follower_inst_ids;
    }

    set<string>& GlobalContext::get_inst_ids_set() {
        return this->inst_ids_set;
    }
    
    moodycamel::ConcurrentQueue<string> *GlobalContext::get_benchmark_ticker_channel() {
        return &(this->benchmark_ticker_channel);
    }
    moodycamel::ConcurrentQueue<string> *GlobalContext::get_follower_ticker_channel() {
        return &(this->follower_ticker_channel);
    }
    moodycamel::ConcurrentQueue<UmTickerInfo> *GlobalContext::get_ticker_info_channel() {
        return &(this->ticker_info_channel);
    }

    moodycamel::ConcurrentQueue<std::string> *GlobalContext::get_early_run_calculation_channel() {
        return &(this->early_run_calculation_channel);
    }

    moodycamel::ConcurrentQueue<std::string> *GlobalContext::get_beta_calculation_channel() {
        return &(this->beta_calculation_channel);
    }
}