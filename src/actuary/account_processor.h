#ifndef _ACTUARY_ACCOUNT_PROCESSOR_H_
#define _ACTUARY_ACCOUNT_PROCESSOR_H_

#include <cstdlib> // For exit()
#include <thread>
#include "config/actuary_config.h"
#include "global_context.h"
#include "binancecpp/binance_futures.h"
#include "binancecpp/binance_spot.h"

namespace actuary {
    void prepare_account_settings(ActuaryConfig &config, GlobalContext &context);
    bool toggle_bnb_burn(ActuaryConfig &config, GlobalContext &context);
    bool change_init_leverage(ActuaryConfig &config, GlobalContext &context);
    bool change_margin_type(ActuaryConfig &config, GlobalContext &context);
    bool change_multi_assets_margin(ActuaryConfig &config, GlobalContext &context);
    bool change_position_side_dual(ActuaryConfig &config, GlobalContext &context);

    void start_polling_load_balance_position(ActuaryConfig &config, GlobalContext &context);
    void load_balance_position(ActuaryConfig &config, GlobalContext &context);
}

#endif