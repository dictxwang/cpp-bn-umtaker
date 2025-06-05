#ifndef _ACTUARY_RISK_CONTROL_H_
#define _ACTUARY_RISK_CONTROL_H_

#include <thread>
#include <chrono>
#include <optional>
#include "config/actuary_config.h"
#include "global_context.h"
#include "logger/logger.h"

using namespace std;

namespace actuary {

    void start_watchdog(ActuaryConfig& config, GlobalContext& context);

    void watch_account_meta(ActuaryConfig& config, GlobalContext& context);
    void watch_bnb_balance(ActuaryConfig& config, GlobalContext& context);

    void send_warning_message(ActuaryConfig& config, GlobalContext& context, string message);
}

#endif