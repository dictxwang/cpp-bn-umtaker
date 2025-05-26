#ifndef _COMMON_THRESHOLD_CONTAINER_H_
#define _COMMON_THRESHOLD_CONTAINER_H_

#include <vector>
#include <cstdint>
#include <mutex>
#include <shared_mutex>
#include <unordered_map>
#include <string>
#include <optional>

using namespace std;

struct EarlyRunThreshold {
    double avg_price_diff_median = 0;
    double bid_ask_price_diff_median = 0;  // benchmark_bid vs follower_ask
    double ask_bid_price_diff_median = 0;  // benchmark_ask vs follower_bid
    size_t price_offset_length = 0;
    uint64_t current_time_mills = 0;
};

struct BetaThreshold {
    double sma = 0;
    double volatility = 0;
    double volatility_multiplier = 0;
    double beta_threshold = 0;

    double bid_sma = 0;
    double bid_volatility = 0;
    double bid_volatility_multiplier = 0;
    double bid_beta_threshold = 0;

    double ask_sma = 0;
    double ask_volatility = 0;
    double ask_volatility_multiplier = 0;
    double ask_beta_threshold = 0;

    uint64_t current_time_mills = 0;
};

class EarlyRunThresholdComposite {
public:
    EarlyRunThresholdComposite() {}
    ~EarlyRunThresholdComposite() {}

private:
    unordered_map<string, EarlyRunThreshold> threshold_map;
    shared_mutex rw_lock;
public:
    void init(vector<string> &base_assets);
    void update(string &base_asset, EarlyRunThreshold& threshold);
    optional<EarlyRunThreshold> get_threshold(string &base_asset);
};

class BetaThresholdComposite {
    public:
        BetaThresholdComposite() {}
        ~BetaThresholdComposite() {}
    
    private:
        unordered_map<string, BetaThreshold> threshold_map;
        shared_mutex rw_lock;
    public:
        void init(vector<string> &base_assets);
        void update(string &base_asset, BetaThreshold& threshold);
        optional<BetaThreshold> get_threshold(string &base_asset);
    };
    
#endif