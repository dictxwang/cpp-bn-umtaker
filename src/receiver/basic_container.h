#ifndef _RECEIVER_BASIC_CONTAINER_H_
#define _RECEIVER_BASIC_CONTAINER_H_

#include <cstdint>
#include <string>
#include "shm/threshold_shm.h"
#include "shm/ticker_shm.h"

using namespace std;

namespace receiver {

    const string TICKER_SOURCE_NORMAL = "normal";
    const string TICKER_SOURCE_ZMQ = "zmq";
    const string TICKER_SOURCE_UDP = "udp";

    struct UmTickerInfo {
        string inst_id;
        double bid_price = 0;
        double bid_volume = 0;
        double ask_price = 0;
        double ask_volume = 0;
        double avg_price = 0;
        uint64_t update_time_millis = 0;
        bool is_from_trade = false;

        void copy_self(UmTickerInfo& other);
    };

    struct ShmStoreInfo {
        int early_run_shm_id;
        shm_mng::EarlyRunThresholdShm* early_run_start;
        int benchmark_beta_shm_id;
        shm_mng::BetaThresholdShm* benchmark_beta_start;
        int follower_beta_shm_id;
        shm_mng::BetaThresholdShm* follower_beta_start;

        int benchmark_shm_id;
        shm_mng::TickerInfoShm* benchmark_start;
        int follower_shm_id;
        shm_mng::TickerInfoShm* follower_start;
    };
}

#endif