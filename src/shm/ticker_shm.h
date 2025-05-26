#ifndef _SHM_TICKER_H_
#define _SHM_TICKER_H_

#include "share_memory_mng.h"

namespace shm_mng {

    struct TickerInfoShm {
        char inst_id[24];
        double bid_price = 0;
        double bid_size = 0;
        double ask_price = 0;
        double ask_size = 0;
        long update_id = 0;
        long update_time = 0;
        long version_number = 0;
        std::atomic_int lock = std::atomic_int(0);
    };

    TickerInfoShm* ticker_shm_find_start_address(int shm_id);
    void ticker_shm_writer_init(TickerInfoShm* start, int offset, const char* asset);
    int ticker_shm_writer_update(TickerInfoShm* start, int offset, TickerInfoShm& threshold);

    std::shared_ptr<TickerInfoShm> ticker_shm_reader_get(TickerInfoShm* start, int offset);
    void ticker_shm_reader_detach(TickerInfoShm* start);
}

#endif