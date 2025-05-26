#ifndef _ACTUARY_GLOBAL_CONTEXT_H_
#define _ACTUARY_GLOBAL_CONTEXT_H_

#include "config/actuary_config.h"
#include "config/inst_config.h"
#include "logger/logger.h"
#include "basic_container.h"
#include <unordered_map>
#include <vector>
#include <set>

using namespace std;

namespace actuary {

    class GlobalContext {
    public:
        GlobalContext() {};
        ~GlobalContext() {};
        
    private:
        InstConfig inst_config;
        vector<string> benchmark_inst_ids;
        vector<string> follower_inst_ids;
        set<string> inst_ids_set;
        unordered_map<string, int> shm_threshold_mapping;
        unordered_map<string, int> shm_benchmark_ticker_mapping;
        unordered_map<string, int> shm_follower_ticker_mapping;

        ShmStoreInfo shm_store_info;
    
    public:
        void init(ActuaryConfig& config);
        void init_shm(ActuaryConfig& config);
        void init_shm_mapping(ActuaryConfig& config);
    };
}
#endif