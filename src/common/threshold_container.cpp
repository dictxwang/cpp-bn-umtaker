#include "threshold_container.h"

using namespace std;

void EarlyRunThresholdComposite::init(vector<string> &base_assets) {
    for (string base : base_assets) {
        // shared_ptr<EarlyRunThreshold> shared_shreshold(new EarlyRunThreshold());
        EarlyRunThreshold threshold;
        this->threshold_map.insert({base, threshold});
    }
}

void EarlyRunThresholdComposite::update(string &base_asset, EarlyRunThreshold &threshold) {
    
    // shared_ptr<EarlyRunThreshold> shared_shreshold(new EarlyRunThreshold());
    // shared_shreshold.get()->avg_price_diff_median = threshold.avg_price_diff_median;
    // shared_shreshold.get()->bid_ask_price_diff_median = threshold.bid_ask_price_diff_median;
    // shared_shreshold.get()->ask_bid_price_diff_median = threshold.ask_bid_price_diff_median;
    // shared_shreshold.get()->price_offset_length = threshold.price_offset_length;
    // shared_shreshold.get()->current_time_mills = threshold.current_time_mills;

    std::unique_lock<std::shared_mutex> w_lock(rw_lock);
    this->threshold_map[base_asset] = threshold;
    w_lock.unlock();
}

optional<EarlyRunThreshold> EarlyRunThresholdComposite::get_threshold(string &base_asset) {
    std::shared_lock<std::shared_mutex> r_lock(rw_lock);
    auto shared = this->threshold_map.find(base_asset);
    if (shared != this->threshold_map.end()) {
        // auto& [key, value] = *shared;

        EarlyRunThreshold threshold;
        threshold.avg_price_diff_median = shared->second.avg_price_diff_median;
        threshold.bid_ask_price_diff_median = shared->second.bid_ask_price_diff_median;
        threshold.ask_bid_price_diff_median = shared->second.ask_bid_price_diff_median;
        threshold.price_offset_length = shared->second.price_offset_length;
        threshold.current_time_mills = shared->second.current_time_mills;
        return threshold;
    } else {
        return nullopt;
    }
}


void BetaThresholdComposite::init(vector<string> &base_assets) {
    for (string base : base_assets) {
        BetaThreshold threshold;
        this->threshold_map.insert({base, threshold});
    }
}

void BetaThresholdComposite::update(string &base_asset, BetaThreshold &threshold) {

    std::unique_lock<std::shared_mutex> w_lock(rw_lock);
    this->threshold_map[base_asset] = threshold;
    w_lock.unlock();
}

optional<BetaThreshold> BetaThresholdComposite::get_threshold(string &base_asset) {
    std::shared_lock<std::shared_mutex> r_lock(rw_lock);
    auto shared = this->threshold_map.find(base_asset);
    if (shared != this->threshold_map.end()) {
        // auto& [key, value] = *shared;

        BetaThreshold threshold;
        threshold.sma = shared->second.sma;
        threshold.volatility = shared->second.volatility;
        threshold.volatility_multiplier = shared->second.volatility_multiplier;
        threshold.beta_threshold = shared->second.beta_threshold;
        threshold.bid_sma = shared->second.bid_sma;
        threshold.bid_volatility = shared->second.bid_volatility;
        threshold.bid_volatility_multiplier = shared->second.bid_volatility_multiplier;
        threshold.bid_beta_threshold = shared->second.bid_beta_threshold;
        threshold.ask_sma = shared->second.ask_sma;
        threshold.ask_volatility = shared->second.ask_volatility;
        threshold.ask_volatility_multiplier = shared->second.ask_volatility_multiplier;
        threshold.ask_beta_threshold = shared->second.ask_beta_threshold;
        threshold.current_time_mills = shared->second.current_time_mills;
        return threshold;
    } else {
        return nullopt;
    }
}