#ifndef _TRADER_SERVICE_MANAGER_H_
#define _TRADER_SERVICE_MANAGER_H_

#include "binancecpp/binance_ws_futures.h"
#include "binancecpp/moodycamel/concurrentqueue.h"
#include "config/trader_config.h"
#include <unordered_map>
#include <vector>
#include <set>
#include <memory>
#include <mutex>
#include <shared_mutex>

using namespace std;

namespace trader {

    class WsClientWrapper {
    public:
        WsClientWrapper() {
            this->is_stopped = true;
        }
        ~WsClientWrapper() {}
    
    private:
        binance::BinanceFuturesWsClient ws_client;
        bool is_stopped;
    
    private:
        void init_and_start(TraderConfig &config);
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
        unordered_map<string, shared_ptr<binance::BinanceFuturesWsClient>> ippair_service_map;
        bool is_started;
        shared_mutex rw_lock;
    
    private:
        void start_polling_best_path();
        void start_subscribe_best_path();
        void start_zookeeper(); // interval checking service alive status and clear unused services
        void update_best_service(string &symbol, string &ip_pair, shared_ptr<binance::BinanceFuturesWsClient>);
    
    public:
        void start_management(TraderConfig &config);
        optional<shared_ptr<binance::BinanceFuturesWsClient>> find_best_service(string &symbol);
    };
}

#endif
