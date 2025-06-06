#ifndef _RECEIVER_TICKER_SUBSCRIBE_H_
#define _RECEIVER_TICKER_SUBSCRIBE_H_

#include "common/constant.h"
#include "common/tools.h"
#include "common/random.h"
#include "common/shm_udp_ticker.container.h"
#include "logger/logger.h"
#include "zmq/zmq_client.h"
#include "config/receiver_config.h"
#include "receiver/global_context.h"
#include "shm/ticker_shm.h"
#include "protocol/ticker_info.pb.h"
#include "binancecpp/binance_ws_futures.h"
#include "binancecpp/util/string_helper.h"
#include "binancecpp/binance_ws_model.h"
#include <thread>
#include <chrono>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/mman.h>

namespace receiver {

    void process_normal_ticker_message(ReceiverConfig& config, GlobalContext &context, TickerRole role);
    void subscribe_normal_ticker(ReceiverConfig& config, GlobalContext& context, vector<string> &inst_ids, TickerRole role);
    void start_subscribe_normal_ticker(ReceiverConfig& config, GlobalContext& context);

    void subscribe_process_zmq_best_ticker(ReceiverConfig& config, GlobalContext& context, size_t ipc_index);
    void start_subscribe_zmq_best_ticker(ReceiverConfig& config, GlobalContext& context);

    void subscribe_process_udp_ticker(ReceiverConfig& config, GlobalContext& context, size_t ipc_index);
    void start_subscribe_udp_ticker(ReceiverConfig& config, GlobalContext& context);
}

#endif