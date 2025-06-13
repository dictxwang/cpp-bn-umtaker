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

    class WsClientWrapper {
    public:
        WsClientWrapper() {
            this->id = binance::generate_uuid();
            this->is_stopped = make_shared<bool>(false);
        }
        ~WsClientWrapper() {
            if (this->order_channel) {
                string temp;
                while (this->order_channel != nullptr && this->order_channel->try_dequeue(temp)) {}
            }
        }
    
    private:
        string id;
        shared_ptr<binance::BinanceFuturesWsClient> ws_client;
        shared_ptr<moodycamel::ConcurrentQueue<string>> order_channel;
        shared_ptr<bool> is_stopped;
    
    private:
        void init_and_start(TraderConfig &config, string &local_ip, string &remote_ip);
        void stop();

        void start_process_trade_thread(const string service_id, shared_ptr<bool> is_stopped, shared_ptr<binance::BinanceFuturesWsClient> futures_ws_client);
        void start_process_message_thread(const string service_id, shared_ptr<bool> is_stopped, shared_ptr<moodycamel::ConcurrentQueue<string>> order_channel);

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
        void update_best_service(TraderConfig &config, string &symbol, string &local_ip, string &remote_ip, shared_ptr<WsClientWrapper>);
        optional<shared_ptr<WsClientWrapper>> find_best_service(string &symbol);
    };
}

#endif