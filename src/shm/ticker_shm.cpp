#include "ticker_shm.h"

namespace shm_mng {

    TickerInfoShm* ticker_shm_find_start_address(int shm_id) {
        TickerInfoShm *start;

        start = (TickerInfoShm*)shmat(shm_id, NULL, 0);
        return start;
    }

    void ticker_shm_writer_init(TickerInfoShm* start, int offset, const char* inst_id) {

        strncpy((start + offset)->inst_id, inst_id, sizeof((start + offset)->inst_id) - 1);
        (start + offset)->inst_id[sizeof((start + offset)->inst_id) - 1] = '\0';
        (start + offset)->bid_price = 0.0;
        (start + offset)->bid_size = 0.0;
        (start + offset)->ask_price = 0.0;
        (start + offset)->ask_size = 0.0;
        (start + offset)->update_id = 0;
        (start + offset)->update_time = 0;
        (start + offset)->version_number = 0;

        atomic_init(&((start + offset)->lock), 0);
    }

    int ticker_shm_writer_update(TickerInfoShm* start, int offset, TickerInfoShm& threshold) {

        long hist_update_id = (*(start + offset)).update_id;
        long hist_update_time = (*(start + offset)).update_time;

        if (hist_update_id < threshold.update_id || hist_update_time < threshold.update_time) {
            int update_result = 0;

            common_acquire_lock(&((start + offset)->lock));

            hist_update_id = (*(start + offset)).update_id;
            hist_update_time = (*(start + offset)).update_time;
            if (hist_update_id < threshold.update_id || hist_update_time < threshold.update_time)
            {
                if (threshold.bid_price > 0 && threshold.bid_size > 0) {
                    (start + offset)->bid_price = threshold.bid_price;
                    (start + offset)->bid_size = threshold.bid_size;
                }
                if (threshold.ask_price > 0 && threshold.ask_size > 0) {
                    (start + offset)->ask_price = threshold.ask_price;
                    (start + offset)->ask_size = threshold.ask_size;
                }

                (start + offset)->update_id = threshold.update_id;
                (start + offset)->update_time = threshold.update_time;

                long version_number = (*(start + offset)).version_number;
                (start + offset)->version_number = version_number + 1;

                if (hist_update_id > 0 && hist_update_time > 0) {
                    update_result = 1;
                }
            }
            common_release_lock(&((start + offset)->lock));

            return update_result;
        } else {
            return 0;
        }
    }

    std::shared_ptr<TickerInfoShm> ticker_shm_reader_get(TickerInfoShm* start, int offset) {
        std::shared_ptr<TickerInfoShm> shared_instance(new TickerInfoShm());
        common_acquire_lock(&((start + offset)->lock));

        // return a new instance
        strncpy((*shared_instance).inst_id, (start + offset)->inst_id, sizeof((*shared_instance).inst_id) - 1);
        (*shared_instance).inst_id[sizeof((*shared_instance).inst_id) - 1] = '\0';
        (*shared_instance).bid_price = (start + offset)->bid_price;
        (*shared_instance).bid_size = (start + offset)->bid_size;
        (*shared_instance).ask_price = (start + offset)->ask_price;
        (*shared_instance).ask_size = (start + offset)->ask_size;
        (*shared_instance).update_id = (start + offset)->update_id;
        (*shared_instance).update_time = (start + offset)->update_time;
        (*shared_instance).version_number = (start + offset)->version_number;
    
        common_release_lock(&((start + offset)->lock));

        return shared_instance;
    }

    void ticker_shm_reader_detach(TickerInfoShm* start) {
        shmdt(start);
    }
}