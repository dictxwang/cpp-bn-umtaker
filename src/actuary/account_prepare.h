#ifndef _ACTUARY_ACCOUNT_PREPARE_H_
#define _ACTUARY_ACCOUNT_PREPARE_H_

#include <stdlib.h>
#include "config/actuary_config.h"
#include "global_context.h"
#include "binancecpp/binance_futures.h"
#include "binancecpp/binance_spot.h"

namespace actuary {
    void load_balabce_position(ActuaryConfig &config, GlobalContext &context);

    void process_account_settings(ActuaryConfig &config, GlobalContext &context);
    bool toggle_bnb_burn(ActuaryConfig &config, GlobalContext &context);
    bool change_init_leverage(ActuaryConfig &config, GlobalContext &context);
    bool change_margin_type(ActuaryConfig &config, GlobalContext &context);
    bool change_multi_assets_margin(ActuaryConfig &config, GlobalContext &context);
    bool change_position_side_dual(ActuaryConfig &config, GlobalContext &context);
}

#endif