#ifndef _RECEIVER_TICKER_CONTAINER_H_
#define _RECEIVER_TICKER_CONTAINER_H_

#include <deque>
#include <vector>
#include <cstdint>
#include <limits>
#include <mutex>
#include <shared_mutex>
#include <unordered_map>
#include <string>
#include <optional>
#include <memory>
#include "basic_container.h"

using namespace std;

namespace receiver {

    struct TickerMinMaxResult {
        double min_bid_price = std::numeric_limits<double>::max();
        double max_bid_price = std::numeric_limits<double>::min();
        double min_ask_price = std::numeric_limits<double>::max();
        double max_ask_price = std::numeric_limits<double>::min();
        double min_avg_price = std::numeric_limits<double>::max();
        double max_avg_price = std::numeric_limits<double>::min();
        size_t ticker_length = 0;
        bool validity_min_max = false;
    };

    class TickerWrapper {

    public:
        TickerWrapper() {
            this->clear_min_max();
        }
        ~TickerWrapper() {}

        void update_ticker(UmTickerInfo ticker, uint64_t remain_senconds);
        optional<UmTickerInfo> get_lastest_ticker();
        deque<UmTickerInfo>& get_ticker_list();
        vector<UmTickerInfo> copy_ticker_list_after(string &inst_id, uint64_t remain_ts_after);
        TickerMinMaxResult get_min_max_result();
        void set_min_max_result(TickerMinMaxResult& result);
    private:
        deque<UmTickerInfo> ticker_list;

        double min_bid_price = std::numeric_limits<double>::max();
        double max_bid_price = std::numeric_limits<double>::min();
        double min_ask_price = std::numeric_limits<double>::max();
        double max_ask_price = std::numeric_limits<double>::min();
        double min_avg_price = std::numeric_limits<double>::max();
        double max_avg_price = std::numeric_limits<double>::min();
        bool validity_min_max = false;

        shared_mutex rw_lock;
    private:
        void clear_min_max();
    };

    class TickerComposite {
    public:
        TickerComposite() {}
        ~TickerComposite() {}
    
    private:
        unordered_map<string, shared_ptr<TickerWrapper>> wrapper_map;
        uint64_t ticker_remain_senconds;
        shared_mutex rw_lock;
    
    public:
        void init(string& quote_asset, vector<string>& base_assets, uint64_t remain_senconds);
        void update_ticker(UmTickerInfo &ticker);
        optional<UmTickerInfo> get_lastest_ticker(string &inst_id);
        vector<UmTickerInfo> copy_ticker_list_after(string &inst_id, uint64_t remain_ts_after);
        
        optional<TickerMinMaxResult> get_min_max_result(string &inst_id);
        void set_min_max_result(string &inst_id, TickerMinMaxResult& result);
    private:
        void init_wrapper(string inst_id);
    };
}

#endif