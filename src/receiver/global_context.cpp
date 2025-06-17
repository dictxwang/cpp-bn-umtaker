#include "global_context.h"

namespace receiver {
    
    void GlobalContext::init(ReceiverConfig& config) {
        this->benchmark_ticker_composite.init(config.benchmark_quote_asset, config.node_base_assets, config.calculate_sma_interval_seconds);
        this->follower_ticker_composite.init(config.follower_quote_asset, config.node_base_assets, config.calculate_sma_interval_seconds);

        this->price_offset_composite.init(config.node_base_assets, config.calculate_sma_interval_seconds);
        this->early_run_threshold_composite.init(config.node_base_assets);
        this->benchmark_beta_threshold_composite.init(config.node_base_assets);
        this->follower_beta_threshold_composite.init(config.node_base_assets);

        for (string base : config.node_base_assets) {
            string benchmark_inst = base + config.benchmark_quote_asset;
            string follower_inst = base + config.follower_quote_asset;
            this->benchmark_inst_ids.push_back(benchmark_inst);
            this->follower_inst_ids.push_back(follower_inst);
            this->inst_ids_set.insert(benchmark_inst);
            this->inst_ids_set.insert(follower_inst);
        }

        for (string base : config.all_base_assets) {
            string benchmark_inst = base + config.benchmark_quote_asset;
            string follower_inst = base + config.follower_quote_asset;
            this->all_benchmark_inst_ids.push_back(benchmark_inst);
            this->all_follower_inst_ids.push_back(follower_inst);
        }

        this->benchmark_inst_config.loadInstConfig(config.benchmark_inst_config_file);
        info_log("finiish load benchmark inst config file.");
        this->follower_inst_config.loadInstConfig(config.follower_inst_config_file);
        info_log("finiish load follower inst config file.");

        this->init_shm_mapping(config);
        if (config.group_main_node) {
            this->init_shm_for_main(config);
            info_log("init share memory for main node");
        } else {
            this->init_shm_for_secondary(config);
            info_log("init share memory for secondary node");
        }

        this->benchmark_ticker_channel = make_shared<moodycamel::ConcurrentQueue<std::string>>();
        this->follower_ticker_channel = make_shared<moodycamel::ConcurrentQueue<std::string>>();
    };

    void GlobalContext::init_shm_for_main(ReceiverConfig& config) {
        ShmStoreInfo info;

        int early_run_shm_id = shm_mng::writer_common_create_shm(config.share_memory_path_early_run.c_str(), config.share_memory_project_id, sizeof(shm_mng::EarlyRunThresholdShm), config.all_base_assets.size());
        shm_mng::EarlyRunThresholdShm* early_run_start = shm_mng::early_run_shm_find_start_address(early_run_shm_id);
        info_log("create early run shm {} start {}", early_run_shm_id, int64_t(early_run_start));

        int benchmark_beta_shm_id = shm_mng::writer_common_create_shm(config.share_memory_path_benchmark_beta.c_str(), config.share_memory_project_id, sizeof(shm_mng::BetaThresholdShm), config.all_base_assets.size());
        shm_mng::BetaThresholdShm* benchmark_beta_start = shm_mng::beta_shm_find_start_address(benchmark_beta_shm_id);
        info_log("create benchmark beta shm {} start {}", benchmark_beta_shm_id, int64_t(benchmark_beta_start));

        int follower_beta_shm_id = shm_mng::writer_common_create_shm(config.share_memory_path_follower_beta.c_str(), config.share_memory_project_id, sizeof(shm_mng::BetaThresholdShm), config.all_base_assets.size());
        shm_mng::BetaThresholdShm* follower_beta_start = shm_mng::beta_shm_find_start_address(follower_beta_shm_id);
        info_log("create follower beta shm {} start {}", follower_beta_shm_id, int64_t(follower_beta_start));

        for (const auto& [key, value] : this->shm_threshold_mapping) {
            shm_mng::early_run_shm_writer_init(early_run_start, value, key.c_str());
            shm_mng::beta_shm_writer_init(benchmark_beta_start, value, key.c_str());
            shm_mng::beta_shm_writer_init(follower_beta_start, value, key.c_str());
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
        info.benchmark_beta_shm_id = benchmark_beta_shm_id;
        info.benchmark_beta_start = benchmark_beta_start;
        info.follower_beta_shm_id = follower_beta_shm_id;
        info.follower_beta_start = follower_beta_start;
        info.benchmark_shm_id = benchmark_shm_id;
        info.benchmark_start = benchmark_start;
        info.follower_shm_id = follower_shm_id;
        info.follower_start = follower_start;

        this->shm_store_info = info;
    }

    void GlobalContext::init_shm_for_secondary(ReceiverConfig& config) {
        ShmStoreInfo info;

        int early_run_shm_id = shm_mng::reader_common_attach_shm(config.share_memory_path_early_run.c_str(), config.share_memory_project_id);
        shm_mng::EarlyRunThresholdShm* early_run_start = shm_mng::early_run_shm_find_start_address(early_run_shm_id);
        info_log("attach early run shm {} start {}", early_run_shm_id, int64_t(early_run_start));

        int benchmark_beta_shm_id = shm_mng::reader_common_attach_shm(config.share_memory_path_benchmark_beta.c_str(), config.share_memory_project_id);
        shm_mng::BetaThresholdShm* benchmark_beta_start = shm_mng::beta_shm_find_start_address(benchmark_beta_shm_id);
        info_log("attach benchmark beta shm {} start {}", benchmark_beta_shm_id, int64_t(benchmark_beta_start));

        int follower_beta_shm_id = shm_mng::reader_common_attach_shm(config.share_memory_path_follower_beta.c_str(), config.share_memory_project_id);
        shm_mng::BetaThresholdShm* follower_beta_start = shm_mng::beta_shm_find_start_address(follower_beta_shm_id);
        info_log("attach follower beta shm {} start {}", follower_beta_shm_id, int64_t(follower_beta_start));

        int benchmark_shm_id = shm_mng::reader_common_attach_shm(config.share_memory_path_benchmark_ticker.c_str(), config.share_memory_project_id);
        shm_mng::TickerInfoShm* benchmark_start = shm_mng::ticker_shm_find_start_address(benchmark_shm_id);
        info_log("attach benchmark shm {} start {}", benchmark_shm_id, int64_t(benchmark_start));
        
        int follower_shm_id = shm_mng::reader_common_attach_shm(config.share_memory_path_follower_ticker.c_str(), config.share_memory_project_id);
        shm_mng::TickerInfoShm* follower_start = shm_mng::ticker_shm_find_start_address(follower_shm_id);
        info_log("attach follower shm {} start {}", follower_shm_id, int64_t(follower_start));

        info.early_run_shm_id = early_run_shm_id;
        info.early_run_start = early_run_start;
        info.benchmark_beta_shm_id = benchmark_beta_shm_id;
        info.benchmark_beta_start = benchmark_beta_start;
        info.follower_beta_shm_id = follower_beta_shm_id;
        info.follower_beta_start = follower_beta_start;
        info.benchmark_shm_id = benchmark_shm_id;
        info.benchmark_start = benchmark_start;
        info.follower_shm_id = follower_shm_id;
        info.follower_start = follower_start;

        this->shm_store_info = info;
    }

    void GlobalContext::init_shm_mapping(ReceiverConfig& config) {
        std::vector<string> sorted_assets;
        for (size_t i = 0; i < config.all_base_assets.size(); ++i) {
            sorted_assets.push_back(config.all_base_assets[i]);
        }
        std::sort(sorted_assets.begin(), sorted_assets.end());
        for (int i = 0; i < sorted_assets.size(); i++) {
            this->shm_threshold_mapping[sorted_assets[i]] = i;
        }

        std::vector<string> sorted_benchmark;
        for (size_t i = 0; i < this->all_benchmark_inst_ids.size(); ++i) {
            sorted_benchmark.push_back(this->all_benchmark_inst_ids[i]);
        }
        std::sort(sorted_benchmark.begin(), sorted_benchmark.end());
        for (int i = 0; i < sorted_benchmark.size(); i++) {
            this->shm_benchmark_ticker_mapping[sorted_benchmark[i]] = i;
        }

        std::vector<string> sorted_follower;
        for (size_t i = 0; i < this->all_follower_inst_ids.size(); ++i) {
            sorted_follower.push_back(this->all_follower_inst_ids[i]);
        }
        std::sort(sorted_follower.begin(), sorted_follower.end());
        for (int i = 0; i < sorted_follower.size(); i++) {
            this->shm_follower_ticker_mapping[sorted_follower[i]] = i;
        }
    }

    InstConfig& GlobalContext::get_benchmark_inst_config() {
        return this->benchmark_inst_config;
    }

    InstConfig& GlobalContext::get_follower_inst_config() {
        return this->follower_inst_config;
    }

    EarlyRunThresholdComposite& GlobalContext::get_early_run_threshold_composite() {
        return this->early_run_threshold_composite;
    }

    BetaThresholdComposite& GlobalContext::get_benchmark_beta_threshold_composite() {
        return this->benchmark_beta_threshold_composite;
    }

    BetaThresholdComposite& GlobalContext::get_follower_beta_threshold_composite() {
        return this->follower_beta_threshold_composite;
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
    
    shared_ptr<moodycamel::ConcurrentQueue<std::string>> GlobalContext::get_benchmark_ticker_channel() {
        return this->benchmark_ticker_channel;
    }
    shared_ptr<moodycamel::ConcurrentQueue<std::string>> GlobalContext::get_follower_ticker_channel() {
        return this->follower_ticker_channel;
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