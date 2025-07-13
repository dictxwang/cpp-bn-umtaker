#ifndef _EVALUATOR_ORDER_STAT_MANAGER_H_
#define _EVALUATOR_ORDER_STAT_MANAGER_H_

#include "config/evaluator_config.h"
#include "global_context.h"
#include "binancecpp/binance_enum.h"
#include "binancecpp/util/string_helper.h"
#include "common/tools.h"

namespace evaluator {

    void load_recently_order_stat_result(EvaluatorConfig& config, GlobalContext& context);
    void polling_calculate_order_pnl(EvaluatorConfig& config, GlobalContext& context);
    void start_order_stat_manager(EvaluatorConfig& config, GlobalContext& context);
}

#endif