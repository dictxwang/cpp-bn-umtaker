#ifndef _TRADER_GLOBAL_CONTEXT_H_
#define _TRADER_GLOBAL_CONTEXT_H_

#include "binancecpp/binance_ws_futures.h"
#include "binancecpp/moodycamel/concurrentqueue.h"
#include "basic_container.h"
#include "config/trader_config.h"
#include "service_container.h"
#include "logger/logger.h"
#include "common/auto_reset_counter.h"
#include "order_limiter.h"
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

        OrderServiceManager best_order_service_manager;

        binance::BinanceFuturesWsClient normal_order_service;
        shared_ptr<moodycamel::ConcurrentQueue<std::string>> order_channel;

        AutoResetCounterBoss order_normal_limiter;
        shared_ptr<AutoResetCounter> order_normal_second_limiter;
        shared_ptr<AutoResetCounter> order_normal_minute_limiter;

        shared_ptr<AutoResetOrderLimiterBoss> order_best_path_limiter;

        shared_ptr<AutoResetOrderIntervalBoss> order_interval_boss;
    
    public:
        void init(TraderConfig& config);
        void init_shm_mapping(TraderConfig& config);
        void init_shm(TraderConfig& config);
        void init_normal_order_limiter(TraderConfig& config);
        void init_best_path_order_limiter(TraderConfig& config);
        void init_normal_order_service(TraderConfig& config);
        void init_order_interval_boss(TraderConfig& config);
        vector<string>& get_benchmark_inst_ids();
        vector<string>& get_follower_inst_ids();
        set<string>& get_follower_inst_id_set();
        ShmStoreInfo& get_shm_store_info();
        unordered_map<string, int>& get_shm_order_mapping();
        OrderServiceManager &get_best_order_service_manager();
        binance::BinanceFuturesWsClient& get_normal_order_service();
        shared_ptr<moodycamel::ConcurrentQueue<std::string>> get_order_channel();
        shared_ptr<AutoResetCounter> get_order_normal_second_limiter();
        shared_ptr<AutoResetCounter> get_order_normal_minute_limiter();
        shared_ptr<AutoResetOrderLimiterBoss> get_order_best_path_limiter();
        shared_ptr<AutoResetOrderIntervalBoss> get_order_interval_boss();
    };
}
#endif
