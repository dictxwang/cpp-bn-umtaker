#include "ticker_container.h"

namespace evaluator {

    void TickerLite::copy_self(TickerLite& lite) {
        lite.symbol = this->symbol;
        lite.ask = this->ask;
        lite.bid = this->bid;
        lite.update_id = this->update_id;
        lite.update_time_millis = lite.update_time_millis;
    }

    void TickerLiteComposite::update_ticker(TickerLite lite) {

        std::unique_lock<std::shared_mutex> lock(rw_lock);
        auto original = this->ticker_map.find(lite.symbol);
        if (original == this->ticker_map.end()) {
            this->ticker_map[lite.symbol] = lite;
        } else {
            if (original->second.update_id < lite.update_id) {
                this->ticker_map[lite.symbol] = lite;
            }
        }
    }

    optional<TickerLite> TickerLiteComposite::get_ticker(string symbol) {

        std::shared_lock<std::shared_mutex> lock(rw_lock);
        auto original = this->ticker_map.find(symbol);
        if (original == this->ticker_map.end()) {
            return nullopt;
        } else {
            TickerLite lite;
            original->second.copy_self(lite);
            return lite;
        }
    }
}