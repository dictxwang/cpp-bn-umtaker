#ifndef _BESTPATH_MARKET_PROCESSOR_H_
#define _BESTPATH_MARKET_PROCESSOR_H_

#include <algorithm>
#include "common_container.h"
#include "config/actuary_config.h"
#include "binancecpp/binance_futures.h"
#include "logger/logger.h"
#include "common/tools.h"

namespace actuary {
    std::vector<ExchangeInfoLite> load_exchangeInfo(ActuaryConfig &config, binance::BinanceFuturesRestClient &restClient);
}

#endif