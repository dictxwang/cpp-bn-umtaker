#ifndef _SHM_order_H_
#define _SHM_order_H_

#include "share_memory_mng.h"

namespace shm_mng {

    struct OrderShm {
        char inst_id[24];
        char side[8];
        char pos_side[8];
        char time_in_force[8];
        double price = 0;
        double volume = 0;
        long update_time = 0;
        long version_number = 0;
        std::atomic_int lock = std::atomic_int(0);
    };

    OrderShm* order_shm_find_start_address(int shm_id);
    void order_shm_writer_init(OrderShm* start, int offset, const char* inst_id);
    int order_shm_writer_update(OrderShm* start, int offset, OrderShm& order);

    std::shared_ptr<OrderShm> order_shm_reader_get(OrderShm* start, int offset);
    void order_shm_reader_detach(OrderShm* start);

}
#endif