#ifndef _EVALUATOR_ZMQ_SUBSCRIBE_H_
#define _EVALUATOR_ZMQ_SUBSCRIBE_H_

#include "config/evaluator_config.h"
#include "global_context.h"
#include "zmq/zmq_client.h"
#include "binancecpp/util/string_helper.h"

using namespace std;

namespace evaluator {

    void start_order_stat_zmq(EvaluatorConfig& config, GlobalContext& context);
    void subscribe_process_order_stat_zmq(EvaluatorConfig& config, GlobalContext& context, size_t ipc_index);

}

#endif