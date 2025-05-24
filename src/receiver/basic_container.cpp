#include "basic_container.h"

namespace receiver {

    void UmTickerInfo::copy_self(UmTickerInfo& other) {
        other.inst_id = this->inst_id;
        other.bid_price = this->bid_price;
        other.bid_volume = this->bid_volume;
        other.ask_price = this->ask_price;
        other.ask_volume = this->ask_volume;
        other.avg_price = this->avg_price;
        other.update_time_millis = this->update_time_millis;
        other.is_from_trade = this->is_from_trade;
    }
}