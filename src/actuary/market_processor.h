#ifndef _BESTPATH_MARKET_PROCESSOR_H_
#define _BESTPATH_MARKET_PROCESSOR_H_

#include <algorithm>
#include "common_container.h"
#include "config/actuary_config.h"
#include "binancecpp/binance_futures.h"
#include "logger/logger.h"
#include "common/tools.h"

namespace actuary {
    vector<ExchangeInfoLite> load_follower_exchangeInfo(ActuaryConfig &config, binance::BinanceFuturesRestClient &restClient);
    unordered_map<string, ExchangeInfoLite> load_benchmark_exchangeInfo(ActuaryConfig &config, binance::BinanceFuturesRestClient &restClient);
}

#endif