#ifndef _ACTUARY_STRATEGY_PROCESSOR_H_
#define _ACTUARY_STRATEGY_PROCESSOR_H_

#include "config/actuary_config.h"
#include "global_context.h"
#include "binancecpp/binance_enum.h"

namespace actuary {

    void start_strategy_process(ActuaryConfig& config, GlobalContext& context);
    void check_signal_make_order(ActuaryConfig& config, GlobalContext& context, std::string& base_asset);

}
#endif