#ifndef _ACTUARY_GLOBAL_CONTEXT_H_
#define _ACTUARY_GLOBAL_CONTEXT_H_

#include "binancecpp/binance_futures.h"
#include "binancecpp/moodycamel/concurrentqueue.h"
#include "config/actuary_config.h"
#include "config/inst_config.h"
#include "logger/logger.h"
#include "basic_container.h"
#include "account_container.h"
#include "shm/threshold_shm.h"
#include "shm/ticker_shm.h"
#include "shm/order_shm.h"
#include "tgbot/api.h"
#include <unordered_map>
#include <memory>
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
        unordered_map<string, int> shm_order_mapping;

        ShmStoreInfo shm_store_info;

        binance::BinanceFuturesRestClient furures_rest_client;
        AccountBalancePositionComposite balance_position_composite;
    
        moodycamel::ConcurrentQueue<string> account_info_channel;
        shared_ptr<string> user_stream_listen_key;
        shared_mutex rw_lock;
        
        tgbot::TgApi tg_bot;

    public:
        void init(ActuaryConfig& config);
        void init_shm_mapping(ActuaryConfig& config);
        void init_shm(ActuaryConfig& config);
        InstConfig &get_inst_config();
        vector<string>& get_benchmark_inst_ids();
        vector<string>& get_follower_inst_ids();
        set<string>& get_inst_ids_set();
        ShmStoreInfo& get_shm_store_info();
        unordered_map<string, int>& get_shm_threshold_mapping();
        unordered_map<string, int>& get_shm_benchmark_ticker_mapping();
        unordered_map<string, int>& get_shm_follower_ticker_mapping();
        unordered_map<string, int>& get_shm_order_mapping();
        binance::BinanceFuturesRestClient& get_furures_rest_client();
        AccountBalancePositionComposite& get_balance_position_composite();

        moodycamel::ConcurrentQueue<string>* get_account_info_channel();
        string get_listen_key();
        void set_listen_key(string listen_key);
        tgbot::TgApi& get_tg_bot();
    };
}
#endif