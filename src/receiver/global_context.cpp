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
        info_log("finiish load inst config file.");

        this->init_shm_mapping(config);
        this->init_shm(config);
    };

    void GlobalContext::init_shm(ReceiverConfig& config) {
        ShmStoreInfo info;

        int early_run_shm_id = shm_mng::writer_common_create_shm(config.share_memory_path_early_run.c_str(), config.share_memory_project_id, SMALL_SEG_PER_SIZE, config.base_asset_list.size());
        shm_mng::EarlyRunThresholdShm* early_run_start = shm_mng::early_run_shm_find_start_address(early_run_shm_id);
        info_log("create early run shm {} start {}", early_run_shm_id, int64_t(early_run_start));

        int beta_shm_id = shm_mng::writer_common_create_shm(config.share_memory_path_beta.c_str(), config.share_memory_project_id, sizeof(shm_mng::BetaThresholdShm), config.base_asset_list.size());
        shm_mng::BetaThresholdShm* beta_start = shm_mng::beta_shm_find_start_address(beta_shm_id);
        info_log("create beta shm {} start {}", beta_shm_id, int64_t(beta_start));

        for (const auto& [key, value] : this->shm_threshold_mapping) {
            shm_mng::early_run_shm_writer_init(early_run_start, value, key.c_str());
            shm_mng::beta_shm_writer_init(beta_start, value, key.c_str());
            info_log("init threshold shm for {} at {}", key, value);
        }

        int benchmark_shm_id = shm_mng::writer_common_create_shm(config.share_memory_path_benchmark_ticker.c_str(), config.share_memory_project_id, sizeof(shm_mng::TickerInfoShm), this->benchmark_inst_ids.size());
        shm_mng::TickerInfoShm* benchmark_start = shm_mng::ticker_shm_find_start_address(benchmark_shm_id);
        info_log("create benchmark shm {} start {}", benchmark_shm_id, int64_t(benchmark_start));
        for (const auto& [key, value] : this->shm_benchmark_ticker_mapping) {
            shm_mng::ticker_shm_writer_init(benchmark_start, value, key.c_str());
            info_log("init benchmark shm for {} at {}", key, value);
        }

        int follower_shm_id = shm_mng::writer_common_create_shm(config.share_memory_path_follower_ticker.c_str(), config.share_memory_project_id, sizeof(shm_mng::TickerInfoShm), this->follower_inst_ids.size());
        shm_mng::TickerInfoShm* follower_start = shm_mng::ticker_shm_find_start_address(follower_shm_id);
        info_log("create follower shm {} start {}", follower_shm_id, int64_t(follower_start));
        for (const auto& [key, value] : this->shm_follower_ticker_mapping) {
            shm_mng::ticker_shm_writer_init(follower_start, value, key.c_str());
            info_log("init follower shm for {} at {}", key, value);
        }

        info.early_run_shm_id = early_run_shm_id;
        info.early_run_start = early_run_start;
        info.beta_shm_id = beta_shm_id;
        info.beta_start = beta_start;
        info.benchmark_shm_id = benchmark_shm_id;
        info.benchmark_start = benchmark_start;
        info.follower_shm_id = follower_shm_id;
        info.follower_start = follower_start;

        this->shm_store_info = info;
    }

    void GlobalContext::init_shm_mapping(ReceiverConfig& config) {
        std::vector<string> sorted_assets;
        for (size_t i = 0; i < config.base_asset_list.size(); ++i) {
            sorted_assets.push_back(config.base_asset_list[i]);
        }
        std::sort(sorted_assets.begin(), sorted_assets.end());
        for (int i = 0; i < sorted_assets.size(); i++) {
            this->shm_threshold_mapping[sorted_assets[i]] = i;
        }

        std::vector<string> sorted_bechmark;
        for (size_t i = 0; i < this->benchmark_inst_ids.size(); ++i) {
            sorted_bechmark.push_back(this->benchmark_inst_ids[i]);
        }
        std::sort(sorted_bechmark.begin(), sorted_bechmark.end());
        for (int i = 0; i < sorted_bechmark.size(); i++) {
            this->shm_benchmark_ticker_mapping[sorted_bechmark[i]] = i;
        }

        std::vector<string> sorted_follower;
        for (size_t i = 0; i < this->follower_inst_ids.size(); ++i) {
            sorted_follower.push_back(this->follower_inst_ids[i]);
        }
        std::sort(sorted_follower.begin(), sorted_follower.end());
        for (int i = 0; i < sorted_follower.size(); i++) {
            this->shm_follower_ticker_mapping[sorted_follower[i]] = i;
        }
    }

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

    ShmStoreInfo& GlobalContext::get_shm_store_info() {
        return this->shm_store_info;
    }

    unordered_map<string, int>& GlobalContext::get_shm_threshold_mapping() {
        return this->shm_threshold_mapping;
    }

    unordered_map<string, int>& GlobalContext::get_shm_benchmark_ticker_mapping() {
        return this->shm_benchmark_ticker_mapping;
    }

    unordered_map<string, int>& GlobalContext::get_shm_follower_ticker_mapping() {
        return this->shm_follower_ticker_mapping;
    }
}