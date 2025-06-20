#include "service_container.h"

using namespace std;

namespace trader {

    void WsClientWrapper::init(TraderConfig &config, shared_ptr<moodycamel::ConcurrentQueue<string>> order_channel, string &local_ip, string &remote_ip) {
        
        this->ws_client = make_shared<binance::BinanceFuturesWsClient>();
        this->ws_client->setLocalIP(local_ip);
        this->ws_client->setRemoteIP(remote_ip);
        this->ws_client->initOrderService(config.api_key_ed25519, config.secret_key_ed25519, config.trade_use_intranet);
        this->ws_client->setMessageChannel(order_channel);
        this->local_ip = local_ip;
    }

    void WsClientWrapper::start() {
        
        std::unique_lock<std::shared_mutex> w_lock(rw_lock);

        // avoid start repeatly
        if (has_init_started) {
            return;
        }

        (*this->is_stopped) = false;

        thread order_process_thread(start_process_trade_thread, this->id, this->is_stopped, this->ws_client);
        order_process_thread.detach();

        has_init_started = true;
        info_log("start order process thread for service {}", this->id);
    }

    void start_process_trade_thread(const string service_id, shared_ptr<bool> is_stopped, shared_ptr<binance::BinanceFuturesWsClient> futures_ws_client) {

        // if order service not be stopped, should restart for itself when error occur
        while (!(*is_stopped)) {
            info_log("order service {} process thread will start", service_id);
            pair<bool, string> result = futures_ws_client->startOrderService();
            warn_log("order service {} process thread return {} {}", service_id, result.first, result.second);

            this_thread::sleep_for(chrono::seconds(1));
        }
        info_log("order service {} processor thread stopped", service_id);
    }

    string& WsClientWrapper::get_local_ip() {
        return this->local_ip;
    }

    void WsClientWrapper::stop() {
        (*this->is_stopped) = true;
        this->ws_client->stop();
        info_log("stop order service ws client wrapper {}", this->id);
    }

    pair<bool, string> WsClientWrapper::place_order(binance::FuturesNewOrder &order) {
        if ((*this->is_stopped)) {
            warn_log("order service has stopped: for place order {}", order.newClientOrderId);
            return pair<bool, string>(false, "service has stopped");
        } else {
            try {
                return this->ws_client->placeOrder(order);
            } catch (exception &exp) {
                err_log("exception occur while place order: {}", string(exp.what()));
                return pair<bool, string>(false, string(exp.what()));
            }
        }
    }

    bool OrderServiceManager::update_best_service(TraderConfig& config, string &symbol, string &local_ip, string &remote_ip, shared_ptr<WsClientWrapper> new_wrapper, shared_ptr<moodycamel::ConcurrentQueue<string>> order_channel, string method) {
        
        std::unique_lock<std::shared_mutex> w_lock(rw_lock);

        string ip_pair = local_ip + "_" + remote_ip;
        auto original = this->ippair_service_map.find(ip_pair);
        bool new_service = false;

        uint64_t now = binance::get_current_ms_epoch();
        if (original == this->ippair_service_map.end()) {
            // not exists original service, need create a new one
            new_wrapper->init(config, order_channel, local_ip, remote_ip);
            new_wrapper->update_time_millis = now;
            new_wrapper->start();
            this->ippair_service_map[ip_pair] = new_wrapper;
            new_service = true;

            info_log("put new order service {} wrapper for {}", new_wrapper->id, ip_pair);
        } else {
            original->second->update_time_millis = now;
            info_log("refresh old order service {} wrapper for {}", new_wrapper->id, ip_pair);
        }

        this->symbol_best_ippair_map[symbol] = ip_pair;
        info_log("update symbol order service mapping {} => {} by {}", symbol, ip_pair, method);

        return new_service;
    }

    optional<shared_ptr<WsClientWrapper>> OrderServiceManager::find_best_service(string &symbol) {

        string ip_pair = "";
        std::shared_lock<std::shared_mutex> lock_key(rw_lock);
        auto ippair_kv = this->symbol_best_ippair_map.find(symbol);
        if (ippair_kv != this->symbol_best_ippair_map.end()) {
            ip_pair = ippair_kv->second;
        }
        lock_key.unlock();

        if (ip_pair.size() == 0) {
            return nullopt;
        }

        
        std::shared_lock<std::shared_mutex> lock_value(rw_lock);
        auto service_kv = this->ippair_service_map.find(ip_pair);
        if (service_kv == this->ippair_service_map.end()) {
            return nullopt;
        } else {
            return service_kv->second;
        }
    }

    vector<string> OrderServiceManager::get_all_service_ip_pairs() {
        vector<string> ip_pairs;
        std::shared_lock<std::shared_mutex> r_lock(rw_lock);
        for (auto [k, _] : this->ippair_service_map) {
            ip_pairs.push_back(k);
        }
        return ip_pairs;
    }

    unordered_map<string, string> OrderServiceManager::get_inuse_symol_ip_mapping() {
        unordered_map<string, string> mapping;
        std::shared_lock<std::shared_mutex> r_lock(rw_lock);
        for (auto [k, v] : this->symbol_best_ippair_map) {
            mapping[k] = v;
        }
        return mapping;
    }

    vector<string> OrderServiceManager::find_update_expired_service(uint64_t expired_millis){

        vector<string> ip_pairs;
        std::shared_lock<std::shared_mutex> r_lock(rw_lock);
        uint64_t now = binance::get_current_ms_epoch();
        for (auto [k, v] : this->ippair_service_map) {
            if (v->update_time_millis + expired_millis <= now) {
                warn_log("order service {} update expired {} now={}", v->id, v->update_time_millis, now);
                ip_pairs.push_back(k);
            }
        }
        return ip_pairs;
    }

    void OrderServiceManager::stop_and_remove_service(const string& ip_pair) {

        std::unique_lock<std::shared_mutex> w_lock(rw_lock);
        auto original = this->ippair_service_map.find(ip_pair);
        if (original != this->ippair_service_map.end()) {
            if (!(*original->second->is_stopped)) {
                original->second->stop();
            }
            this->ippair_service_map.erase(ip_pair);
        }
    }
}