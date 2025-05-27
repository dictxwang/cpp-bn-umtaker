#include "threshold_shm.h"

namespace shm_mng {

    EarlyRunThresholdShm* early_run_shm_find_start_address(int shm_id) {
        
        EarlyRunThresholdShm *start;
        start = (EarlyRunThresholdShm*)shmat(shm_id, NULL, 0);
        return start;
    }

    void early_run_shm_writer_init(EarlyRunThresholdShm* start, int offset, const char* asset) {

        // EarlyRunThresholdShm* item_list = static_cast<EarlyRunThresholdShm*>(start);
        // item_list[0].avg_median = 0.0;

        // strncpy((start + offset)->asset, asset, sizeof((start + offset)->asset) - 1);
        // (start + offset)->asset[sizeof((start + offset)->asset) - 1] = '\0';
        memcpy((start + offset)->asset, asset, strlen(asset) + 1);
        (start + offset)->avg_median = 0.0;
        (start + offset)->bid_ask_median = 0.0;
        (start + offset)->ask_bid_median = 0.0;
        (start + offset)->version_number = 0;
        (start + offset)->time_mills = 0;

        atomic_init(&((start + offset)->lock), 0);
    }

    int early_run_shm_writer_update(EarlyRunThresholdShm* start, int offset, EarlyRunThresholdShm& threshold) {

        long hist_time_mills = (*(start + offset)).time_mills;
    
        if (hist_time_mills <= threshold.time_mills) {
            int update_result = 0;
    
            common_acquire_lock(&((start + offset)->lock));
    
            hist_time_mills = (*(start + offset)).time_mills;
            if (hist_time_mills <= threshold.time_mills) {

                (start + offset)->avg_median = threshold.avg_median;
                (start + offset)->bid_ask_median = threshold.bid_ask_median;
                (start + offset)->ask_bid_median = threshold.ask_bid_median;
                (start + offset)->time_mills = threshold.time_mills;
                long version_number = (*(start + offset)).version_number;
                (start + offset)->version_number = version_number + 1;
                
                update_result = 1;
            }
            common_release_lock(&((start + offset)->lock));
    
            return update_result;
        } else {
            return 0;
        }
    }

    std::shared_ptr<EarlyRunThresholdShm> early_run_shm_reader_get(EarlyRunThresholdShm* start, int offset) {
        std::shared_ptr<EarlyRunThresholdShm> shared_instance(new EarlyRunThresholdShm());
        common_acquire_lock(&((start + offset)->lock));

        // return a new instance
        strncpy((*shared_instance).asset, (start + offset)->asset, sizeof((*shared_instance).asset) - 1);
        (*shared_instance).asset[sizeof((*shared_instance).asset) - 1] = '\0';
        (*shared_instance).avg_median = (start + offset)->avg_median;
        (*shared_instance).bid_ask_median = (start + offset)->bid_ask_median;
        (*shared_instance).ask_bid_median = (start + offset)->ask_bid_median;
        (*shared_instance).version_number = (start + offset)->version_number;
        (*shared_instance).time_mills = (start + offset)->time_mills;
    
        common_release_lock(&((start + offset)->lock));

        return shared_instance;
    }

    void early_run_shm_reader_detach(EarlyRunThresholdShm* start) {
        shmdt(start);
    }

    BetaThresholdShm* beta_shm_find_start_address(int shm_id) {

        BetaThresholdShm *start;
        start = (BetaThresholdShm*)shmat(shm_id, NULL, 0);
        return start;
    }

    void beta_shm_writer_init(BetaThresholdShm* start, int offset, const char* asset) {

        // strncpy((start + offset)->asset, asset, sizeof((start + offset)->asset) - 1);
        // (start + offset)->asset[sizeof((start + offset)->asset) - 1] = '\0';
        memcpy((start + offset)->asset, asset, strlen(asset) + 1);
        (start + offset)->sma = 0.0;
        (start + offset)->volatility = 0.0;
        (start + offset)->volatility_multiplier = 0.0;
        (start + offset)->beta_threshold = 0.0;

        (start + offset)->bid_sma = 0.0;
        (start + offset)->bid_volatility = 0.0;
        (start + offset)->bid_volatility_multiplier = 0.0;
        (start + offset)->bid_beta_threshold = 0.0;

        (start + offset)->ask_sma = 0.0;
        (start + offset)->ask_volatility = 0.0;
        (start + offset)->ask_volatility_multiplier = 0.0;
        (start + offset)->ask_beta_threshold = 0.0;

        (start + offset)->version_number = 0;
        (start + offset)->time_mills = 0;

        atomic_init(&((start + offset)->lock), 0);
    }
    int beta_shm_writer_update(BetaThresholdShm* start, int offset, BetaThresholdShm& threshold) {

        long hist_time_mills = (*(start + offset)).time_mills;
    
        if (hist_time_mills <= threshold.time_mills) {
            int update_result = 0;
    
            common_acquire_lock(&((start + offset)->lock));
    
            hist_time_mills = (*(start + offset)).time_mills;
            if (hist_time_mills <= threshold.time_mills) {

                (start + offset)->sma = threshold.sma;
                (start + offset)->volatility = threshold.volatility;
                (start + offset)->volatility_multiplier = threshold.volatility_multiplier;
                (start + offset)->beta_threshold = threshold.beta_threshold;

                (start + offset)->bid_sma = threshold.bid_sma;
                (start + offset)->bid_volatility = threshold.bid_volatility;
                (start + offset)->bid_volatility_multiplier = threshold.bid_volatility_multiplier;
                (start + offset)->bid_beta_threshold = threshold.bid_beta_threshold;

                (start + offset)->ask_sma = threshold.ask_sma;
                (start + offset)->ask_volatility = threshold.ask_volatility;
                (start + offset)->ask_volatility_multiplier = threshold.ask_volatility_multiplier;
                (start + offset)->ask_beta_threshold = threshold.ask_beta_threshold;

                (start + offset)->time_mills = threshold.time_mills;
                long version_number = (*(start + offset)).version_number;
                (start + offset)->version_number = version_number + 1;
                
                update_result = 1;
            }
            common_release_lock(&((start + offset)->lock));
    
            return update_result;
        } else {
            return 0;
        }
    }

    std::shared_ptr<BetaThresholdShm> beta_shm_reader_get(BetaThresholdShm* start, int offset) {

        std::shared_ptr<BetaThresholdShm> shared_instance(new BetaThresholdShm());
        common_acquire_lock(&((start + offset)->lock));

        // return a new instance
        strncpy((*shared_instance).asset, (start + offset)->asset, sizeof((*shared_instance).asset) - 1);
        (*shared_instance).asset[sizeof((*shared_instance).asset) - 1] = '\0';
        (*shared_instance).sma = (start + offset)->sma;
        (*shared_instance).volatility = (start + offset)->volatility;
        (*shared_instance).volatility_multiplier = (start + offset)->volatility_multiplier;
        (*shared_instance).beta_threshold = (start + offset)->beta_threshold;

        (*shared_instance).bid_sma = (start + offset)->bid_sma;
        (*shared_instance).bid_volatility = (start + offset)->bid_volatility;
        (*shared_instance).bid_volatility_multiplier = (start + offset)->bid_volatility_multiplier;
        (*shared_instance).bid_beta_threshold = (start + offset)->bid_beta_threshold;

        (*shared_instance).ask_sma = (start + offset)->ask_sma;
        (*shared_instance).ask_volatility = (start + offset)->ask_volatility;
        (*shared_instance).ask_volatility_multiplier = (start + offset)->ask_volatility_multiplier;
        (*shared_instance).ask_beta_threshold = (start + offset)->ask_beta_threshold;

        (*shared_instance).version_number = (start + offset)->version_number;
        (*shared_instance).time_mills = (start + offset)->time_mills;
    
        common_release_lock(&((start + offset)->lock));

        return shared_instance;
    }

    void beta_shm_reader_detach(BetaThresholdShm* start) {
        shmdt(start);
    }
}