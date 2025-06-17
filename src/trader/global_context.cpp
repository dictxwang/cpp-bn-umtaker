#include "global_context.h"

namespace trader {

    void GlobalContext::init(TraderConfig& config) {

        for (string base : config.node_base_assets) {
            string benchmark_inst = base + config.benchmark_quote_asset;
            string follower_inst = base + config.follower_quote_asset;
            this->benchmark_inst_ids.push_back(benchmark_inst);
            this->follower_inst_ids.push_back(follower_inst);
            this->follower_inst_id_set.insert(follower_inst);
        }

        for (string base : config.all_base_assets) {
            string follower_inst = base + config.follower_quote_asset;
            this->all_follower_inst_ids.push_back(follower_inst);
        }

        this->order_channel = make_shared<moodycamel::ConcurrentQueue<std::string>>();

        this->init_shm_mapping(config);
        this->init_shm(config);
        info_log("finish init share memory settings");

        // init trading service
        this->init_order_service(config);
        info_log("finish init order service");

        // init auto reset counter
        this->counter_boss.init(10, 1);
        this->api_second_limiter = this->counter_boss.create_new_counter(config.order_limit_per_10seconds, 10*1000).value();
        this->api_minute_limiter = this->counter_boss.create_new_counter(config.order_limit_per_minute, 60*1000).value();
        this->counter_boss.start();
    }

    void GlobalContext::init_order_service(TraderConfig& config) {

        if (config.trade_local_ip.size() > 0) {
            this->order_service.setLocalIP(config.trade_local_ip);
        }
        this->order_service.initOrderService(config.api_key_ed25519, config.secret_key_ed25519, config.trade_use_intranet);
        this->order_service.setMessageChannel(this->get_order_channel());
    }

    void GlobalContext::init_shm_mapping(TraderConfig& config) {

        std::vector<string> sorted_follower;
        for (size_t i = 0; i < this->all_follower_inst_ids.size(); ++i) {
            sorted_follower.push_back(this->all_follower_inst_ids[i]);
        }
        std::sort(sorted_follower.begin(), sorted_follower.end());
        for (int i = 0; i < sorted_follower.size(); i++) {
            this->shm_order_mapping[sorted_follower[i]] = i;
        }
    }

    void GlobalContext::init_shm(TraderConfig& config) {

        ShmStoreInfo info;

        int order_shm_id = shm_mng::reader_common_attach_shm(config.share_memory_path_order.c_str(), config.share_memory_project_id);
        shm_mng::OrderShm* order_start = shm_mng::order_shm_find_start_address(order_shm_id);
        info_log("attach order shm {} start {}", order_shm_id, int64_t(order_start));

        info.order_shm_id = order_shm_id;
        info.order_start = order_start;

        this->shm_store_info = info;
    }

    vector<string>& GlobalContext::get_benchmark_inst_ids() {
        return this->benchmark_inst_ids;
    }
    vector<string>& GlobalContext::get_follower_inst_ids() {
        return this->follower_inst_ids;
    }
    set<string>& GlobalContext::get_follower_inst_id_set() {
        return this->follower_inst_id_set;
    }
    ShmStoreInfo& GlobalContext::get_shm_store_info() {
        return this->shm_store_info;
    }
    unordered_map<string, int>& GlobalContext::get_shm_order_mapping() {
        return this->shm_order_mapping;
    }
    OrderServiceManager& GlobalContext::get_order_service_manager() {
        return this->order_service_manager;
    }

    binance::BinanceFuturesWsClient& GlobalContext::get_order_service() {
        return this->order_service;
    }
    shared_ptr<moodycamel::ConcurrentQueue<std::string>> GlobalContext::get_order_channel() {
        return this->order_channel;
    }
    shared_ptr<AutoResetCounter> GlobalContext::get_api_second_limiter() {
        return this->api_second_limiter;
    }
    shared_ptr<AutoResetCounter> GlobalContext::get_api_minute_limiter() {
        return this->api_minute_limiter;
    }
}