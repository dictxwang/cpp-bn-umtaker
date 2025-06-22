#include "ticker_container.h"
#include "binancecpp/util/binance_tool.h"
#include <iostream>

namespace receiver {

    void TickerWrapper::clear_min_max() {
        this->min_bid_price = std::numeric_limits<double>::max();
        this->max_bid_price = std::numeric_limits<double>::min();
        this->min_ask_price = std::numeric_limits<double>::max();
        this->max_ask_price = std::numeric_limits<double>::min();
        this->min_avg_price = std::numeric_limits<double>::max();
        this->max_avg_price = std::numeric_limits<double>::min();
        this->validity_min_max = false;
    }

    void TickerWrapper::update_ticker(UmTickerInfo ticker, uint64_t remain_senconds) {
        
        std::unique_lock<std::shared_mutex> lock(rw_lock);
        // Also could directly lock
        // this->rw_lock.lock();
        ticker_list.push_back(ticker);

        // change min max when which values are validity
        // so when the values are not validity, must wait the ticker calculator make the values validity
        if (this->validity_min_max) {
            if (ticker.bid_price > this->max_bid_price) {
                this->max_bid_price = ticker.bid_price;
            }
            if (ticker.bid_price < this->min_bid_price) {
                this->min_bid_price = ticker.bid_price;
            }
            if (ticker.ask_price > this->max_ask_price) {
                this->max_ask_price = ticker.ask_price;
            }
            if (ticker.ask_price < this->min_ask_price) {
                this->min_ask_price = ticker.ask_price;
            }
            if (ticker.avg_price > this->max_avg_price) {
                this->max_avg_price = ticker.avg_price;
            }
            if (ticker.avg_price < this->min_avg_price) {
                this->min_avg_price = ticker.avg_price;
            }
        }
        
        uint64_t now = binance::get_current_ms_epoch();
        while (!ticker_list.empty()) {
            if (now > ticker_list.front().update_time_millis + remain_senconds * 1000) {
                
                // check if the min or max value is relation with the list 'front' item
                if (ticker_list.front().bid_price >= this->max_bid_price || ticker_list.front().bid_price <= this->min_bid_price
                    || ticker_list.front().ask_price >= this->max_ask_price || ticker_list.front().ask_price <= this->min_ask_price
                    || ticker_list.front().avg_price >= this->max_avg_price || ticker_list.front().avg_price <= this->min_avg_price) {
                    // clear min max value not validity, make ticker calculator work
                    this->clear_min_max();
                }

                // remove expired ticker
                ticker_list.pop_front();
            } else {
                // no more expired ticker
                break;
            }
        }

        // unlock directly
        // this->rw_lock.unlock();
        lock.unlock();
    }

    deque<UmTickerInfo>& TickerWrapper::get_ticker_list() {
        return this->ticker_list;
    }

    optional<UmTickerInfo> TickerWrapper::get_lastest_ticker() {

        // automatically released when `lock` goes out of scope
        std::shared_lock<std::shared_mutex> lock(rw_lock);
        if (!ticker_list.empty()) {
            return ticker_list.back();
        } else {
            return nullopt;
        }
    }

    vector<UmTickerInfo> TickerWrapper::copy_ticker_list_after(string &inst_id, uint64_t remain_ts_after) {
        std::vector<UmTickerInfo> result;
        std::shared_lock<std::shared_mutex> lock(rw_lock);
        for (size_t i = 0; i < this->ticker_list.size(); i++) {
            if (this->ticker_list[i].update_time_millis > remain_ts_after) {
                UmTickerInfo info;
                this->ticker_list[i].copy_self(info);
                result.push_back(info);
            }
        }
        return result;
    }

    TickerMinMaxResult TickerWrapper::get_min_max_result() {
        
        TickerMinMaxResult result;
        std::shared_lock<std::shared_mutex> lock(rw_lock);
        result.min_bid_price = this->min_bid_price;
        result.max_bid_price = this->max_bid_price;
        result.min_ask_price = this->min_ask_price;
        result.max_ask_price = this->max_ask_price;
        result.min_avg_price = this->min_avg_price;
        result.max_avg_price = this->max_avg_price;
        result.ticker_length = this->ticker_list.size();
        result.validity_min_max = this->validity_min_max;

        return result;
    }

    void TickerWrapper::set_min_max_result(TickerMinMaxResult& result) {

        std::unique_lock<std::shared_mutex> lock(rw_lock);
        this->min_bid_price = result.min_bid_price;
        this->max_bid_price = result.max_bid_price;
        this->min_ask_price = result.min_ask_price;
        this->max_ask_price = result.max_ask_price;
        this->min_avg_price = result.min_avg_price;
        this->max_avg_price = result.max_avg_price;
        this->validity_min_max = result.validity_min_max;
    }

    void TickerComposite::init(string& quote_asset, vector<string>& base_assets, uint64_t remain_seconds) {
        this->ticker_remain_senconds = remain_seconds;

        for (string& base : base_assets) {
            string inst_id = base + quote_asset;
            this->init_wrapper(inst_id);
        }
    }

    void TickerComposite::init_wrapper(string inst_id) {
        shared_ptr<TickerWrapper> shared_t_wrapper(new TickerWrapper());
        this->rw_lock.lock();
        this->wrapper_map.insert({inst_id, shared_t_wrapper});
        this->rw_lock.unlock();
    }
    
    void TickerComposite::update_ticker(UmTickerInfo &ticker) {

        // rw_lock.lock_shared();
        // auto wrapper = this->wrapper_map.find(ticker.inst_id);
        // rw_lock.unlock_shared();

        std::shared_lock<std::shared_mutex> r_lock(rw_lock);
        auto wrapper = this->wrapper_map.find(ticker.inst_id);
        r_lock.unlock();

        if (wrapper != this->wrapper_map.end()) {
            auto& [key, value] = *wrapper;
            (*value).update_ticker(ticker, this->ticker_remain_senconds);
            return;
        }

        std::shared_ptr<TickerWrapper> shared_t_wrapper(new TickerWrapper());
        shared_t_wrapper.get()->update_ticker(ticker, this->ticker_remain_senconds);
        std::unique_lock<std::shared_mutex> w_lock(rw_lock);
        wrapper_map.insert({ticker.inst_id, shared_t_wrapper});
        w_lock.unlock();
    }

    optional<UmTickerInfo> TickerComposite::get_lastest_ticker(string &inst_id) {
        std::shared_lock<std::shared_mutex> lock(rw_lock);
        auto wrapper = this->wrapper_map.find(inst_id);
        lock.unlock();

        if (wrapper != this->wrapper_map.end()) {
            return (*(wrapper->second)).get_lastest_ticker();
        } else {
            return nullopt;
        }
    }

    vector<UmTickerInfo> TickerComposite::copy_ticker_list_after(string &inst_id, uint64_t remain_ts_after) {
        std::vector<UmTickerInfo> result;
        std::shared_lock<std::shared_mutex> lock(rw_lock);
        auto wrapper = this->wrapper_map.find(inst_id);
        if (wrapper != this->wrapper_map.end()) {
            result = (*wrapper->second).copy_ticker_list_after(inst_id, remain_ts_after);
        }
        return result;
    }

    optional<TickerMinMaxResult> TickerComposite::get_min_max_result(string &inst_id) {

        std::shared_lock<std::shared_mutex> lock(rw_lock);
        auto wrapper = this->wrapper_map.find(inst_id);
        if (wrapper != this->wrapper_map.end()) {
            return wrapper->second->get_min_max_result();
        } else {
            return nullopt;
        }
    }

    void TickerComposite::set_min_max_result(string &inst_id, TickerMinMaxResult& result) {

        std::shared_lock<std::shared_mutex> lock(rw_lock);
        auto wrapper = this->wrapper_map.find(inst_id);
        if (wrapper != this->wrapper_map.end()) {
            wrapper->second->set_min_max_result(result);
        }
    }
}