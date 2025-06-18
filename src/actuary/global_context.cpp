#include "global_context.h"

namespace actuary {
    
    void GlobalContext::init(ActuaryConfig& config) {
        
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
        this->init_shm(config);

        // init rest client
        if (config.rest_local_ip.size() > 0) {
            this->furures_rest_client.setLocalIP(config.rest_local_ip);
        }
        this->furures_rest_client.init(config.api_key_hmac, config.secret_key_hmac, config.rest_use_intranet);

        // load exchange info
        vector<ExchangeInfoLite> exchangeLites = load_exchangeInfo(config, this->furures_rest_client);
        for (int i = 0; i < exchangeLites.size(); i++) {
            if (this->inst_ids_set.find(exchangeLites[i].symbol) != this->inst_ids_set.end()) {
                info_log("put exchange info of {} into map", exchangeLites[i].symbol);
                this->exchange_info_map.insert({exchangeLites[i].symbol, exchangeLites[i]});
            }
        }
        info_log("finish load exchange info map");

        // init balance & position composite
        std::vector<std::string> balance_assets;
        balance_assets.push_back(config.benchmark_quote_asset);
        balance_assets.push_back(config.follower_quote_asset);
        balance_assets.push_back("BNB");
        for (std::string base : config.all_base_assets) {
            balance_assets.push_back(base);
        }
        this->balance_position_composite.init(balance_assets, this->all_follower_inst_ids);

        this->user_stream_listen_key = make_shared<string>("");

        this->tg_bot.init_default_endpoint(config.tg_bot_token);

        this->dynamic_config = make_shared<DynamicConfig>();

        this->account_info_channel = make_shared<moodycamel::ConcurrentQueue<string>>();

        this->init_db_source(config);
        info_log("finish init db source of mysql");
    }

    void GlobalContext::init_db_source(ActuaryConfig &config) {
        this->mysql_source = make_shared<db_source::MySQLConnectionPool>(
            config.db_host,
            config.db_username,
            config.db_password,
            config.db_database
        );
    }

    void GlobalContext::init_shm_mapping(ActuaryConfig& config) {

        std::vector<string> sorted_assets;
        for (size_t i = 0; i < config.all_base_assets.size(); ++i) {
            sorted_assets.push_back(config.all_base_assets[i]);
        }
        std::sort(sorted_assets.begin(), sorted_assets.end());
        for (int i = 0; i < sorted_assets.size(); i++) {
            this->shm_threshold_mapping[sorted_assets[i]] = i;
        }

        std::vector<string> sorted_bechmark;
        for (size_t i = 0; i < this->all_benchmark_inst_ids.size(); ++i) {
            sorted_bechmark.push_back(this->all_benchmark_inst_ids[i]);
        }
        std::sort(sorted_bechmark.begin(), sorted_bechmark.end());
        for (int i = 0; i < sorted_bechmark.size(); i++) {
            this->shm_benchmark_ticker_mapping[sorted_bechmark[i]] = i;
        }

        std::vector<string> sorted_follower;
        for (size_t i = 0; i < this->all_follower_inst_ids.size(); ++i) {
            sorted_follower.push_back(this->all_follower_inst_ids[i]);
        }
        std::sort(sorted_follower.begin(), sorted_follower.end());
        for (int i = 0; i < sorted_follower.size(); i++) {
            this->shm_follower_ticker_mapping[sorted_follower[i]] = i;
            this->shm_order_mapping[sorted_follower[i]] = i;
        }
    }

    void GlobalContext::init_shm(ActuaryConfig& config) {
        
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

        if (config.group_main_node) {
            int order_shm_id = shm_mng::writer_common_create_shm(config.share_memory_path_order.c_str(), config.share_memory_project_id, sizeof(shm_mng::OrderShm), config.all_base_assets.size());
            shm_mng::OrderShm* order_start = shm_mng::order_shm_find_start_address(order_shm_id);
            info_log("create order shm {} start {}", order_shm_id, int64_t(order_start));

            for (const auto& [key, value] : this->shm_order_mapping) {
                shm_mng::order_shm_writer_init(order_start, value, key.c_str());
                info_log("main node init order shm for {} at {}", key, value);
            }
            info.order_shm_id = order_shm_id;
            info.order_start = order_start;
        } else {
            int order_shm_id = shm_mng::reader_common_attach_shm(config.share_memory_path_order.c_str(), config.share_memory_project_id);
            shm_mng::OrderShm* order_start = shm_mng::order_shm_find_start_address(order_shm_id);
            info.order_shm_id = order_shm_id;
            info.order_start = order_start;
            info_log("secondary node attach order shm {} start {}", order_shm_id, int64_t(order_start));
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

    InstConfig& GlobalContext::get_benchmark_inst_config() {
        return this->benchmark_inst_config;
    }

    InstConfig& GlobalContext::get_follower_inst_config() {
        return this->follower_inst_config;
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

    optional<ExchangeInfoLite> GlobalContext::get_exchange_info(const string& symbol) {
        auto original = this->exchange_info_map.find(symbol);
        if (original == this->exchange_info_map.end()) {
            return nullopt;
        } else {
            return original->second;
        }
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

    unordered_map<string, int>& GlobalContext::get_shm_order_mapping() {
        return this->shm_order_mapping;
    }

    binance::BinanceFuturesRestClient& GlobalContext::get_furures_rest_client() {
        return this->furures_rest_client;
    }

    AccountBalancePositionComposite& GlobalContext::get_balance_position_composite() {
        return this->balance_position_composite;
    }
    shared_ptr<moodycamel::ConcurrentQueue<string>> GlobalContext::get_account_info_channel() {
        return this->account_info_channel;
    }
    string GlobalContext::get_listen_key() {
        std::shared_lock<std::shared_mutex> r_lock(this->rw_lock);
        return (*(this->user_stream_listen_key));
    }
    void GlobalContext::set_listen_key(string listen_key) {
        std::unique_lock<std::shared_mutex> w_lock(this->rw_lock);
        (*(this->user_stream_listen_key)) = listen_key;
    }
    tgbot::TgApi& GlobalContext::get_tg_bot() {
        return this->tg_bot;
    }
    shared_ptr<DynamicConfig> GlobalContext::get_dynamic_config() {
        return this->dynamic_config;
    }

    bool GlobalContext::dynamic_could_make_order() {
        return !(*(this->dynamic_config)).is_stop_make_order();
    }
    
    shared_ptr<db_source::MySQLConnectionPool> GlobalContext::get_mysql_source() {
        return this->mysql_source;
    }
}