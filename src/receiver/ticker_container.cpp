#include "ticker_container.h"
#include "binancecpp/util/binance_tool.h"
#include <iostream>

namespace receiver {

    void TickerWrapper::update_ticker(UmTickerInfo ticker, uint64_t remain_senconds) {
        
        std::unique_lock<std::shared_mutex> lock(rw_lock);
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
        TickerWrapper t_wrapper;
        this->rw_lock.lock();
        this->wrapper_map.insert({inst_id, t_wrapper});
        this->rw_lock.unlock();
    }
    
    void TickerComposite::update_ticker(UmTickerInfo &ticker) {

        try {
            rw_lock.lock_shared();
            auto wrapper = this->wrapper_map.find(ticker.inst_id);
            rw_lock.unlock_shared();


            if (wrapper != this->wrapper_map.end()) {
                try {
                    auto& [key, value] = *wrapper;
                    value.update_ticker(ticker, this->ticker_remain_senconds);
                } catch (std::exception &exp) {
                    std::cout << "innner1 exception: " << exp.what() << ": " << wrapper->second.get_ticker_list().size() << std::endl;
                }
                return;
            }

            TickerWrapper t_wrapper;
            t_wrapper.update_ticker(ticker, this->ticker_remain_senconds);
            std::cout << "ticker info count " << t_wrapper.get_ticker_list().size() << std::endl;
            try {
                // std::unique_lock<std::shared_mutex> lock(rw_lock);
                rw_lock.lock();
                wrapper_map.insert({ticker.inst_id, t_wrapper});
                rw_lock.unlock();
            } catch (std::exception &exp) {
                std::cout << "inner2 exception" << std::endl;
            }

        } catch (std::exception &exp) {
            std::cout << "outer exception" << std::endl;
        }
    }

    optional<UmTickerInfo> TickerComposite::get_lastest_ticker(string &inst_id) {
        rw_lock.lock_shared();
        auto wrapper = this->wrapper_map.find(inst_id);
        rw_lock.unlock_shared();

        if (wrapper != this->wrapper_map.end()) {
            return wrapper->second.get_lastest_ticker();
        } else {
            return nullopt;
        }
    }
}