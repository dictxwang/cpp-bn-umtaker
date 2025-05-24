#ifndef _RECEIVER_OFFSET_CONTAINER_H_
#define _RECEIVER_OFFSET_CONTAINER_H_

#include <deque>
#include <cstdint>
#include <mutex>
#include <shared_mutex>
#include <unordered_map>
#include <string>
#include <optional>
#include <memory>
#include "basic_container.h"
#include "config/receiver_config.h"

using namespace std;

namespace receiver {

    struct PriceOffset {
        double avg_price_diff = 0;
        double bid_ask_price_diff = 0;  // benchmark_bid vs follower_ask
        double ask_bid_price_diff = 0;  // benchmark_ask vs follower_bid
        uint64_t update_time_millis = 0;

        bool is_expired(uint64_t seconds);
        void copy_self(PriceOffset &other);
    };

    struct PriceOffsetWrapper {
        deque<PriceOffset> offset_list = {};
        shared_mutex rw_lock;

        void update_price_offset(UmTickerInfo &benchmark_tick, UmTickerInfo &follower_tick, uint64_t remain_senconds);
        optional<PriceOffset> get_lastest_price_offset();
        vector<PriceOffset> copy_price_offset_list();
    };

    class PriceOffsetComposite {
    public:
        PriceOffsetComposite() {}
        ~PriceOffsetComposite() {}
    private:    
        unordered_map<string, shared_ptr<PriceOffsetWrapper>> wrapper_map;
        uint64_t offset_remain_senconds;
        shared_mutex rw_lock;
    
    public:
        void init(vector<string>& base_assets, uint64_t remain_senconds);
        void update_price_offset(std::string& base_asset, UmTickerInfo &benchmark_tick, UmTickerInfo &follower_tick);
        optional<PriceOffset> get_lastest_price_offset(string &base_asset);
        vector<PriceOffset> get_price_offset_list(string &base_asset);
    private:
        void init_wrapper(string base_asset);
    };
}

#endif