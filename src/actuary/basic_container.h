#ifndef _ACTUARY_BASIC_CONTAINER_H_
#define _ACTUARY_BASIC_CONTAINER_H_

#include "shm/ticker_shm.h"
#include "shm/threshold_shm.h"
#include "shm/order_shm.h"

namespace actuary {

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

        int order_shm_id;
        shm_mng::OrderShm* order_start;
    };
}
#endif