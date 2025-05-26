#ifndef _SHM_TICKER_H_
#define _SHM_TICKER_H_

#include "share_memory_mng.h"

namespace shm_mng {

    struct TickerInfoShm {
        char inst_id[24];
        double bid_price;
        double bid_size;
        double ask_price;
        double ask_size;
        long update_id;
        long update_time;
        long version_number;
        atomic_int lock;
    };

    TickerInfoShm* ticker_shm_find_start_address(int shm_id);
    void ticker_shm_writer_init(TickerInfoShm* start, int offset, const char* asset);
    int ticker_shm_writer_update(TickerInfoShm* start, int offset, TickerInfoShm& threshold);

    TickerInfoShm ticker_shm_reader_get(TickerInfoShm* start, int offset);
    void ticker_shm_reader_detach(TickerInfoShm* start);
}

#endif