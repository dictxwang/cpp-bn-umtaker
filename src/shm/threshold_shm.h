#ifndef _SHM_THRESHOLD_H_
#define _SHM_THRESHOLD_H_

#include "share_memory_mng.h"

namespace shm_mng {

    struct EarlyRunThresholdShm {
        char asset[16];
        double avg_median = 0;
        double bid_ask_median = 0;  // benchmark_bid vs follower_ask
        double ask_bid_median = 0;  // benchmark_ask vs follower_bid
        uint64_t time_mills = 0;
        atomic_int lock = atomic_int(0);
    };

    struct BetaThresholdShm {
        char asset[16];
        double sma = 0;
        double volatility = 0;
        double volatility_multiplier = 0;
        double beta_threshold = 0;
    
        double bid_sma = 0;
        double bid_volatility = 0;
        double bid_volatility_multiplier = 0;
        double bid_beta_threshold = 0;
    
        double ask_sma = 0;
        double ask_volatility = 0;
        double ask_volatility_multiplier = 0;
        double ask_beta_threshold = 0;

        uint64_t time_mills = 0;
        atomic_int lock = atomic_int(0);
    };

    EarlyRunThresholdShm* early_run_shm_find_start_address(int shm_id);
    void early_run_shm_writer_init(EarlyRunThresholdShm* start, int offset, char* asset);
    int early_run_shm_writer_update(EarlyRunThresholdShm* start, int offset, EarlyRunThresholdShm& threshold);

    EarlyRunThresholdShm* early_run_shm_reader_get(EarlyRunThresholdShm* start, int offset);
    void early_run_shm_reader_detach(EarlyRunThresholdShm* start);


    BetaThresholdShm* beta_shm_find_start_address(int shm_id);
    void beta_shm_writer_init(BetaThresholdShm* start, int offset, char* asset);
    int beta_shm_writer_update(BetaThresholdShm* start, int offset, BetaThresholdShm& threshold);

    BetaThresholdShm* beta_shm_reader_get(BetaThresholdShm* start, int offset);
    void beta_shm_reader_detach(BetaThresholdShm* start);
}

#endif