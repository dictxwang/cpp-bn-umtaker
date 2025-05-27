#ifndef _TRADER_GLOBAL_CONTEXT_H_
#define _TRADER_GLOBAL_CONTEXT_H_

#include "basic_container.h"
#include "config/trader_config.h"
#include "logger/logger.h"
#include "shm/order_shm.h"
#include <unordered_map>
#include <vector>
#include <set>

using namespace std;

namespace trader {

    class GlobalContext {
    public:
        GlobalContext() {};
        ~GlobalContext() {};
    
    private:     
        vector<string> benchmark_inst_ids;
        vector<string> follower_inst_ids;
        set<string> inst_ids_set;
        
        unordered_map<string, int> shm_order_mapping;
        ShmStoreInfo shm_store_info;
    
    public:
        void init(TraderConfig& config);
        void init_shm_mapping(TraderConfig& config);
        void init_shm(TraderConfig& config);
        vector<string>& get_benchmark_inst_ids();
        vector<string>& get_follower_inst_ids();
        set<string>& get_inst_ids_set();
        ShmStoreInfo& get_shm_store_info();
        unordered_map<string, int>& get_shm_order_mapping();
    };
}
#endif
