#ifndef _ACTUARY_ACCOUNT_PREPARE_H_
#define _ACTUARY_ACCOUNT_PREPARE_H_

#include "config/actuary_config.h"
#include "binancecpp/binance_futures.h"
#include "binancecpp/binance_spot.h"

namespace actuary {
    void load_account_info(ActuaryConfig &config);
}

#endif