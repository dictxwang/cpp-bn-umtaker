#ifndef _TRADER_GLOBAL_CONTEXT_H_
#define _TRADER_GLOBAL_CONTEXT_H_

#include "binancecpp/binance_ws_futures.h"
#include "binancecpp/moodycamel/concurrentqueue.h"
#include "basic_container.h"
#include "config/trader_config.h"
#include "service_container.h"
#include "logger/logger.h"
#include "common/auto_reset_counter.h"
#include "shm/order_shm.h"
#include <unordered_map>
#include <vector>
#include <set>
#include <memory>

using namespace std;

namespace trader {

    class GlobalContext {
    public:
        GlobalContext() {};
        ~GlobalContext() {};
    
    private:     
        vector<string> benchmark_inst_ids;
        vector<string> follower_inst_ids;
        set<string> follower_inst_id_set;
        vector<std::string> all_follower_inst_ids;
        
        unordered_map<string, int> shm_order_mapping;
        ShmStoreInfo shm_store_info;

        OrderServiceManager order_service_manager;

        binance::BinanceFuturesWsClient order_service;
        shared_ptr<moodycamel::ConcurrentQueue<std::string>> order_channel;

        AutoResetCounterBoss counter_boss;
        shared_ptr<AutoResetCounter> api_second_limiter;
        shared_ptr<AutoResetCounter> api_minute_limiter;
    
    public:
        void init(TraderConfig& config);
        void init_shm_mapping(TraderConfig& config);
        void init_shm(TraderConfig& config);
        void init_order_service(TraderConfig& config);
        vector<string>& get_benchmark_inst_ids();
        vector<string>& get_follower_inst_ids();
        set<string>& get_follower_inst_id_set();
        ShmStoreInfo& get_shm_store_info();
        unordered_map<string, int>& get_shm_order_mapping();
        OrderServiceManager &get_order_service_manager();
        binance::BinanceFuturesWsClient& get_order_service();
        shared_ptr<moodycamel::ConcurrentQueue<std::string>> get_order_channel();
        shared_ptr<AutoResetCounter> get_api_second_limiter();
        shared_ptr<AutoResetCounter> get_api_minute_limiter();
    };
}
#endif
