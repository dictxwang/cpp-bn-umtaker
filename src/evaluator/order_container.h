#ifndef _EVALUATOR_ORDER_CONTAINER_H_
#define _EVALUATOR_ORDER_CONTAINER_H_

#include <string>
#include <cstdint>
#include <mutex>
#include <shared_mutex>
#include <unordered_map>
#include "binancecpp/util/binance_tool.h"

using namespace std;

namespace evaluator {

    struct StatOrderLite {
        string accountFlag;
        string symbol;
        string orderSide;
        string clientOrderId;
        double averagePrice;
        double filledSize;
        double commissionRate;
        uint64_t systemTimestamp = 0;
    };

    struct StatOrderFullInfo {
        string account_flag;
        string symbol;
        string order_side;
        string client_order_id;
        double average_price;
        double filled_size;
        double commission_rate;
        uint64_t system_timestamp = 0;
        bool property_changed = false;

        double price_after_1s = 0;
        double price_after_5s = 0;
        double price_after_10s = 0;
        double price_after_20s = 0;
        double price_after_30s = 0;
        double price_after_40s = 0;
        double price_after_50s = 0;
        double price_after_60s = 0;
        double price_after_120s = 0;
        double price_after_180s = 0;
        double price_after_15min = 0;
        double price_after_30min = 0;
        double price_after_1hour = 0;

        double pnl_1s_percentage = 0;
        double pnl_5s_percentage = 0;
        double pnl_10s_percentage = 0;
        double pnl_20s_percentage = 0;
        double pnl_30s_percentage = 0;
        double pnl_40s_percentage = 0;
        double pnl_50s_percentage = 0;
        double pnl_60s_percentage = 0;
        double pnl_120s_percentage = 0;
        double pnl_180s_percentage = 0;
        double pnl_15min_percentage = 0;
        double pnl_30min_percentage = 0;
        double pnl_1hour_percentage = 0;
    };

    class StatOrderFullInfoComposite {
    public:
        StatOrderFullInfoComposite() {}
        ~StatOrderFullInfoComposite() {}
    
    private:
        unordered_map<string, StatOrderFullInfo> stat_order_map;
        shared_mutex rw_lock;
    
    public:
        bool update_with_full_info(StatOrderFullInfo info);
        bool update_with_lite(StatOrderLite lite);
        vector<StatOrderFullInfo> get_all_stat_orders();
        pair<int, int> remove_expired_stat_order();
    };
}

#endif