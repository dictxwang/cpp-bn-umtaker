#ifndef _EVALUATOR_TICKER_SUBSCRIBE_H_
#define _EVALUATOR_TICKER_SUBSCRIBE_H_

#include "config/evaluator_config.h"
#include "global_context.h"
#include "common/random.h"
#include "binancecpp/binance_ws_futures.h"
#include "binancecpp/util/string_helper.h"
#include "binancecpp/binance_ws_model.h"

namespace evaluator {

    void process_normal_futures_ticker_message(EvaluatorConfig& config, GlobalContext& context);
    void subscribe_normal_futures_ticker(EvaluatorConfig& config, GlobalContext& context);
    void start_subscribe_normal_ticker(EvaluatorConfig& config, GlobalContext& context);

}

#endif