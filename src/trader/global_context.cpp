#include "global_context.h"

namespace trader {

    void GlobalContext::init(TraderConfig& config) {

        for (string base : config.base_asset_list) {
            string benchmark_inst = base + config.benchmark_quote_asset;
            string follower_inst = base + config.follower_quote_asset;
            this->benchmark_inst_ids.push_back(benchmark_inst);
            this->follower_inst_ids.push_back(follower_inst);
            this->inst_ids_set.insert(benchmark_inst);
            this->inst_ids_set.insert(follower_inst);
        }

        this->init_shm_mapping(config);
        this->init_shm(config);
    }

    void GlobalContext::init_shm_mapping(TraderConfig& config) {

        std::vector<string> sorted_follower;
        for (size_t i = 0; i < this->follower_inst_ids.size(); ++i) {
            sorted_follower.push_back(this->follower_inst_ids[i]);
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
    set<string>& GlobalContext::get_inst_ids_set() {
        return this->inst_ids_set;
    }
    ShmStoreInfo& GlobalContext::get_shm_store_info() {
        return this->shm_store_info;
    }
    unordered_map<string, int>& GlobalContext::get_shm_order_mapping() {
        return this->shm_order_mapping;
    }
}