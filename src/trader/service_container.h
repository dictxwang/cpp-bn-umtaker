#ifndef _TRADER_SERVICE_CONTAINER_H_
#define _TRADER_SERVICE_CONTAINER_H_

#include "binancecpp/binance_ws_futures.h"
#include "binancecpp/util/binance_tool.h"
#include "binancecpp/moodycamel/concurrentqueue.h"
#include "config/trader_config.h"
#include "logger/logger.h"
#include <unordered_map>
#include <vector>
#include <set>
#include <memory>
#include <mutex>
#include <thread>
#include <chrono>
#include <shared_mutex>

using namespace std;

namespace trader {

    void start_process_trade_thread(const string service_id, shared_ptr<bool> is_stopped, shared_ptr<binance::BinanceFuturesWsClient> futures_ws_client);
    // void start_process_message_thread(const string service_id, shared_ptr<bool> is_stopped, shared_ptr<moodycamel::ConcurrentQueue<string>> order_channel);

    class WsClientWrapper {
    public:
        WsClientWrapper() {
            this->id = binance::generate_uuid();
            this->has_init_started = false;
            this->is_stopped = make_shared<bool>(false);
            this->update_time_millis = 0;
        }
        ~WsClientWrapper() {
        }
    
    private:
        string id;
        shared_ptr<binance::BinanceFuturesWsClient> ws_client;
        bool has_init_started;
        shared_ptr<bool> is_stopped;
        uint64_t update_time_millis;
        shared_mutex rw_lock;
    
    private:
        void init(TraderConfig &config, shared_ptr<moodycamel::ConcurrentQueue<string>> order_channel, string &local_ip, string &remote_ip);
        void start();
        void stop();

        friend class OrderServiceManager;
    
    public:
        pair<bool, string> place_order(binance::FuturesNewOrder &order);
    };

    class OrderServiceManager {
    public:
        OrderServiceManager() {
            this->is_started = false;
        }
        ~OrderServiceManager() {}
    
    private:
        unordered_map<string, string> symbol_best_ippair_map;
        unordered_map<string, shared_ptr<WsClientWrapper>> ippair_service_map;
        bool is_started;
        shared_mutex rw_lock;

    public:
        void update_best_service(TraderConfig &config, string &symbol, string &local_ip, string &remote_ip, shared_ptr<WsClientWrapper> new_wrapper, shared_ptr<moodycamel::ConcurrentQueue<string>> order_channel);
        optional<shared_ptr<WsClientWrapper>> find_best_service(string &symbol);

        vector<string> get_all_service_ip_pairs();
        unordered_map<string, string> get_inuse_symol_ip_mapping();
        vector<string> find_update_expired_service(uint64_t expired_millis);
        void stop_and_remove_service(const string& ip_pair);
    };
}

#endif