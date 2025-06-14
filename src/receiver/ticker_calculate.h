#ifndef _RECEIVER_TICKER_CALCULATE_H_
#define _RECEIVER_TICKER_CALCULATE_H_

#include <algorithm>
#include <limits>
#include <cstdint>
#include "config/receiver_config.h"
#include "receiver/global_context.h"
#include "common/constant.h"
#include "common/tools.h"
#include "common/random.h"
#include "logger/logger.h"
#include "shm/threshold_shm.h"
#include "binancecpp/util/string_helper.h"
#include "binancecpp/util/binance_tool.h"

namespace receiver {

    void start_ticker_calculate(ReceiverConfig& config, GlobalContext &context);

    void process_ticker_info_price_offset(ReceiverConfig& config, GlobalContext &context);
    void process_early_run_threshold_calculate(ReceiverConfig& config, GlobalContext &context);
    void process_beta_threshold_calculate(ReceiverConfig& config, GlobalContext &context);

    double calculate_volatility_multiplier(double volatility, InstConfigItem &inst_config);
    double calculate_beta_threshold(double volatility_multiplier, InstConfigItem &inst_config);
}

#endif