#ifndef _RECEIVER_TICKER_CONTAINER_H_
#define _RECEIVER_TICKER_CONTAINER_H_

#include <deque>
#include <vector>
#include <cstdint>
#include <mutex>
#include <shared_mutex>
#include <unordered_map>
#include <string>
#include <optional>
#include <memory>
#include "basic_container.h"

using namespace std;

namespace receiver {

    class TickerWrapper {

    public:
        TickerWrapper() {}
        ~TickerWrapper() {}

        void update_ticker(UmTickerInfo ticker, uint64_t remain_senconds);
        optional<UmTickerInfo> get_lastest_ticker();
        deque<UmTickerInfo>& get_ticker_list();
    private:
        deque<UmTickerInfo> ticker_list;
        shared_mutex rw_lock;
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
    private:
        void init_wrapper(string inst_id);
    };
}

#endif