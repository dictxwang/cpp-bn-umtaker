#include "offset_container.h"
#include "binancecpp/util/binance_tool.h"

namespace receiver {

    bool PriceOffset::is_expired(uint64_t seconds) {
        uint64_t now = binance::get_current_ms_epoch();
        return now - this->update_time_millis > seconds * 1000;
    }

    void PriceOffset::copy_self(PriceOffset &other) {
        other.avg_price_diff = this->avg_price_diff;
        other.bid_ask_price_diff = this->bid_ask_price_diff;
        other.ask_bid_price_diff = this->ask_bid_price_diff;
        other.update_time_millis = this->update_time_millis;
    }

    void PriceOffsetWrapper::update_price_offset(UmTickerInfo &benchmark_tick, UmTickerInfo &follower_tick, uint64_t remain_senconds) {

        uint64_t now = binance::get_current_ms_epoch();

        PriceOffset offset;
        offset.avg_price_diff = (benchmark_tick.avg_price - follower_tick.avg_price) / 2;
        offset.bid_ask_price_diff = benchmark_tick.bid_price - follower_tick.ask_price;
        offset.ask_bid_price_diff = benchmark_tick.ask_price - follower_tick.bid_price;
        offset.update_time_millis = now;

        std::unique_lock<std::shared_mutex> lock(rw_lock);
        this->offset_list.push_back(offset);
        while (!offset_list.empty()) {
            if (now - offset_list.front().update_time_millis > remain_senconds * 1000) {
                // remove expired ticker
                offset_list.pop_front();
            } else {
                // no more expired ticker
                break;
            }
        }
        lock.unlock();
    }

    optional<PriceOffset> PriceOffsetWrapper::get_latest_price_offset() {
        std::shared_lock<std::shared_mutex> lock(rw_lock);
        if (!offset_list.empty()) {
            return offset_list.back();
        } else {
            return nullopt;
        }
    }
    
    vector<PriceOffset> PriceOffsetWrapper::copy_price_offset_list() {
        vector<PriceOffset> result;
        std::shared_lock<std::shared_mutex> lock(rw_lock);
        for (size_t i = 0; i < this->offset_list.size(); ++i) {
            PriceOffset other;
            this->offset_list[i].copy_self(other);
            result.push_back(other);
        }
        return result;
    }

    void PriceOffsetComposite::init_wrapper(string base_asset) {
        shared_ptr<PriceOffsetWrapper> shared_t_wrapper(new PriceOffsetWrapper());
        this->rw_lock.lock();
        this->wrapper_map.insert({base_asset, shared_t_wrapper});
        this->rw_lock.unlock();
    }

    void PriceOffsetComposite::init(vector<string>& base_assets, uint64_t remain_senconds) {
        this->offset_remain_senconds = remain_senconds;
        for (string base : base_assets) {
            this->init_wrapper(base);
        }
    }

    void PriceOffsetComposite::update_price_offset(std::string& base_asset, UmTickerInfo &benchmark_tick, UmTickerInfo &follower_tick) {
        
        std::shared_lock<std::shared_mutex> r_lock(rw_lock); 
        auto wrapper = this->wrapper_map.find(base_asset);
        r_lock.unlock();

        if (wrapper != this->wrapper_map.end()) {
            auto& [key, value] = *wrapper;
            (*value).update_price_offset(benchmark_tick, follower_tick, this->offset_remain_senconds);
            return;
        }

        std::shared_ptr<PriceOffsetWrapper> shared_t_wrapper(new PriceOffsetWrapper());
        shared_t_wrapper.get()->update_price_offset(benchmark_tick, follower_tick, this->offset_remain_senconds);
        std::unique_lock<std::shared_mutex> w_lock(rw_lock);
        wrapper_map.insert({base_asset, shared_t_wrapper});
        w_lock.unlock();
    }

    optional<PriceOffset> PriceOffsetComposite::get_latest_price_offset(string &base_asset) {
        std::shared_lock<std::shared_mutex> lock(rw_lock);
        auto wrapper = this->wrapper_map.find(base_asset);
        lock.unlock();

        if (wrapper != this->wrapper_map.end()) {
            return (*(wrapper->second)).get_latest_price_offset();
        } else {
            return nullopt;
        }
        // TODO could auto unlock
    }

    vector<PriceOffset> PriceOffsetComposite::get_price_offset_list(string &base_asset) {

        std::shared_lock<std::shared_mutex> lock(rw_lock);
        auto wrapper = this->wrapper_map.find(base_asset);
        lock.unlock();
        if (wrapper != this->wrapper_map.end()) {
            return (*(wrapper->second)).copy_price_offset_list();
        } else {
            vector<PriceOffset> result;
            return result;
        }
    }
}