#include "ticker_container.h"
#include "binancecpp/util/binance_tool.h"
#include <iostream>

namespace receiver {

    void TickerWrapper::update_ticker(UmTickerInfo ticker, uint64_t remain_senconds) {
        
        std::unique_lock<std::shared_mutex> lock(rw_lock);
        // Also could directly lock
        // this->rw_lock.lock();
        ticker_list.push_back(ticker);

        uint64_t now = binance::get_current_ms_epoch();
        while (!ticker_list.empty()) {
            if (now - ticker_list.front().update_time_millis > remain_senconds * 1000) {
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
            for (size_t i = 0; i < (*(wrapper->second)).get_ticker_list().size(); i++) {
                if ((*(wrapper->second)).get_ticker_list()[i].update_time_millis > remain_ts_after) {
                    UmTickerInfo info;
                    (*(wrapper->second)).get_ticker_list()[i].copy_self(info);
                    result.push_back(info);
                }
            }
        }
        return result;
    }
}