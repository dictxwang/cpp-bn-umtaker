#ifndef _RECEIVER_TICKER_CALCULATE_H_
#define _RECEIVER_TICKER_CALCULATE_H_

#include "config/receiver_config.h"
#include "receiver/global_context.h"
#include "common/tools.h"
#include "common/random.h"
#include "logger/logger.h"
#include "binancecpp/util/string_helper.h"

namespace receiver {

    void start_ticker_calculate(ReceiverConfig& config, GlobalContext &context);
    void process_ticker_info_price_offset(ReceiverConfig& config, GlobalContext &context);
}

#endif