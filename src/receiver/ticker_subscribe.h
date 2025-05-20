#ifndef _RECEIVER_TICKER_SUBSCRIBE_H_
#define _RECEIVER_TICKER_SUBSCRIBE_H_

#include "config/receiver_config.h"
#include "receiver/global_context.h"

namespace receiver {

    void subscribe_normal_ticker(ReceiverConfig& config, GlobalContext& context, vector<string> &inst_ids);
    void start_subscribe_normal_ticker(ReceiverConfig& config, GlobalContext& context);

    void subscribe_best_ticker(GlobalContext& context, string &ipc);
    void start_subscribe_best_ticker(ReceiverConfig& config, GlobalContext& context);
}

#endif