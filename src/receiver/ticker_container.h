#ifndef _RECEIVER_TICKER_CONTAINER_H_
#define _RECEIVER_TICKER_CONTAINER_H_

#include <deque>
#include <cstdint>
#include <shared_mutex>
#include <unordered_map>

using namespace std;

namespace receiver {

    struct UmTickerInfo {
        double bid_price = 0;
        double bid_volume = 0;
        double ask_price = 0;
        double ask_volume = 0;
        double avg_price = 0;
        uint64_t update_time_millis = 0;
        bool is_from_trade = false;
    };

    struct TickerWrapper {
        deque<UmTickerInfo> ticker_list;
        shared_mutex rw_lock;

        void update_ticker(UmTickerInfo ticker, uint64_t remain_senconds);
        optional<UmTickerInfo&> get_lastest_ticker();
    };

    class TickerComposite {
    public:
        TickerComposite() {}
        ~TickerComposite() {}
    
    private:
        unordered_map<string, TickerWrapper&> wrapper_map;
        uint64_t ticker_remain_senconds;
        shared_mutex rw_lock;
    
    public:
        void init(string& quote_asset, vector<string>& base_assets, uint64_t remain_senconds);
        void update_ticker(string inst_id, UmTickerInfo &ticker);
        optional<UmTickerInfo&> get_lastest_ticker(string &inst_id);
    private:
        void init_wrapper(string inst_id);
    };
}

#endif