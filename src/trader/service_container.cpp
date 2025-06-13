#include "service_container.h"

using namespace std;

namespace trader {

    void WsClientWrapper::init_and_start(TraderConfig &config, string &local_ip, string &remote_ip) {
        this->ws_client = make_shared<binance::BinanceFuturesWsClient>();
        this->ws_client->setLocalIP(local_ip);
        this->ws_client->setRemoteIP(remote_ip);
        this->ws_client->initOrderService(config.api_key_ed25519, config.secret_key_ed25519, config.trade_use_intranet);
        (*this->is_stopped) = false;

        shared_ptr<moodycamel::ConcurrentQueue<string>> message_channel;
        this->ws_client->setMessageChannel(message_channel);

        this->order_channel = message_channel;

        thread order_process_thread(start_process_trade_thread, this->id, this->is_stopped, this->ws_client);
        order_process_thread.detach();

        thread message_process_thread(start_process_message_thread, this->id, this->is_stopped, message_channel);
        message_process_thread.detach();
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

    void start_process_message_thread(const string service_id, shared_ptr<bool> is_stopped, shared_ptr<moodycamel::ConcurrentQueue<string>> order_channel) {
        
        if ((*is_stopped)) {
            warn_log("service {} is stopped before process order messaage", service_id);
        } else {
            info_log("service {} is not stopped before process order messaage", service_id);
        }
        while (!(*is_stopped)) {
            std::string messageJson;
            while (!(*is_stopped) && !order_channel->try_dequeue(messageJson)) {
                // Retry if the queue is empty
            }

            if (messageJson.size() == 0) {
                continue;
            }

            // only logging, datastat in actuary process
            info_log("receive order json message: {}", messageJson);
        }

        info_log("order message processor {} stopped", service_id);
    }

    void WsClientWrapper::stop() {
        (*this->is_stopped) = true;
        this->ws_client->stop();
        info_log("stop ws client wrapper {}", this->id);
    }

    pair<bool, string> WsClientWrapper::place_order(binance::FuturesNewOrder &order) {
        if ((*this->is_stopped)) {
            warn_log("order service has stopped: for place order {}", order.newClientOrderId);
            return pair<bool, string>(false, "service has stopped");
        } else {
            return this->ws_client->placeOrder(order);
        }
    }

    void OrderServiceManager::update_best_service(TraderConfig& config, string &symbol, string &local_ip, string &remote_ip, shared_ptr<WsClientWrapper> new_wrapper) {
        
        std::unique_lock<std::shared_mutex> w_lock(rw_lock);

        string ip_pair = local_ip + "_" + remote_ip;
        shared_ptr<WsClientWrapper> original_wrapper = nullptr;
        auto original = this->ippair_service_map.find(ip_pair);
        if (original != this->ippair_service_map.end()) {
            original_wrapper = original->second;
        }

        if (original_wrapper == nullptr) {
            // not exists, need create a new one
            // shared_ptr<WsClientWrapper> new_wrapper = make_shared<WsClientWrapper>();
            new_wrapper->init_and_start(config, local_ip, remote_ip);

            this->ippair_service_map[ip_pair] = new_wrapper;

            info_log("put new service client wrapper for {}", ip_pair);
        }

        this->symbol_best_ippair_map[symbol] = ip_pair;
        info_log("update symbol ip_pair mapping {} => {}", symbol, ip_pair);
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

    set<string> OrderServiceManager::get_in_use_ip_pairs() {
        set<string> ip_pairs;
        std::shared_lock<std::shared_mutex> r_lock(rw_lock);
        for (auto [_, v] : this->symbol_best_ippair_map) {
            ip_pairs.insert(v);
        }
        return ip_pairs;

    }

    void OrderServiceManager::stop_and_remove_service(const string& ip_pair) {

        std::unique_lock<std::shared_mutex> w_lock(rw_lock);
        auto original = this->ippair_service_map.find(ip_pair);
        if (original != this->ippair_service_map.end()) {
            if (!original->second->is_stopped) {
                original->second->stop();
            }
            this->ippair_service_map.erase(ip_pair);
        }
    }
}