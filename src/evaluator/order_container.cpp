#include "order_container.h"

namespace evaluator {

    bool StatOrderFullInfoComposite::update_with_lite(StatOrderLite lite) {

        std::unique_lock<std::shared_mutex> lock(rw_lock);
        auto original = this->stat_order_map.find(lite.clientOrderId);
        if (original != this->stat_order_map.end()) {
            original->second.filled_size = lite.filledSize;
            original->second.average_price = lite.averagePrice;
            // restore into map
            this->stat_order_map[original->first] = original->second;
        } else {
            StatOrderFullInfo info;
            info.account_flag = lite.accountFlag;
            info.symbol = lite.symbol;
            info.order_side = lite.orderSide;
            info.client_order_id = lite.clientOrderId;
            info.average_price = lite.averagePrice;
            info.filled_size = lite.filledSize;
            info.commission_rate = lite.commissionRate;
            info.system_timestamp = lite.systemTimestamp;
            this->stat_order_map[lite.clientOrderId] = info;
        }

        return true;
    }

    bool StatOrderFullInfoComposite::update_with_full_info(StatOrderFullInfo info) {

        uint64_t now = binance::get_current_epoch();

        if (now > info.system_timestamp + 3600) {
            // ignore expired info
            return false;
        } else {
            std::unique_lock<std::shared_mutex> lock(rw_lock);
            this->stat_order_map[info.client_order_id] = info;
            return true;
        }
    }
    
    vector<StatOrderFullInfo> StatOrderFullInfoComposite::get_all_stat_orders() {

        vector<StatOrderFullInfo> result;
        std::shared_lock<std::shared_mutex> lock(rw_lock);
        for (auto kv : this->stat_order_map) {
            result.push_back(kv.second);
        }
        return result;
    }

    pair<int, int> StatOrderFullInfoComposite::remove_expired_stat_order() {

        uint64_t now = binance::get_current_epoch();
        vector<string> need_remove_keys;

        std::shared_lock<std::shared_mutex> r_lock(rw_lock);
        int original_size = this->stat_order_map.size();
        for (auto kv : this->stat_order_map) {
            if (now > kv.second.system_timestamp + 3600 + 10) {
                need_remove_keys.push_back(kv.first);
            }
        }
        r_lock.unlock();

        std::unique_lock<std::shared_mutex> w_lock(rw_lock);
        for (string id : need_remove_keys) {
            this->stat_order_map.erase(id);
        }
        int remain_size = this->stat_order_map.size();
        w_lock.unlock();

        return pair<int, int>(original_size, remain_size);
    }
}