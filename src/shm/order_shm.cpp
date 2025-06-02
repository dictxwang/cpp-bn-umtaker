#include "order_shm.h"

namespace shm_mng {

    OrderShm* order_shm_find_start_address(int shm_id) {

        OrderShm *start;

        start = (OrderShm*)shmat(shm_id, 0, 0);
        return start;
    }

    void order_shm_writer_init(OrderShm* start, int offset, const char* inst_id) {
        memcpy((start + offset)->inst_id, inst_id, strlen(inst_id) + 1);
        (start + offset)->price = 0.0;
        (start + offset)->volume = 0.0;
        (start + offset)->update_time = 0;
        (start + offset)->version_number = 0;
        (start + offset)->reduce_only = 0;

        atomic_init(&((start + offset)->lock), 0);
    }

    int order_shm_writer_update(OrderShm* start, int offset, OrderShm& order) {

        long hist_update_time = (*(start + offset)).update_time;

        if (hist_update_time < order.update_time) {
            int update_result = 0;

            common_acquire_lock(&((start + offset)->lock));
            hist_update_time = (*(start + offset)).update_time;

            if (hist_update_time < order.update_time) {

                memcpy((start + offset)->inst_id, order.inst_id, strlen(order.inst_id) + 1);
                memcpy((start + offset)->type, order.type, strlen(order.type) + 1);
                memcpy((start + offset)->side, order.side, strlen(order.side) + 1);
                memcpy((start + offset)->pos_side, order.pos_side, strlen(order.pos_side) + 1);
                memcpy((start + offset)->time_in_force, order.time_in_force, strlen(order.time_in_force) + 1);
                memcpy((start + offset)->client_order_id, order.client_order_id, strlen(order.client_order_id) + 1);
                (start + offset)->price = order.price;
                (start + offset)->volume = order.volume;
                (start + offset)->reduce_only= order.reduce_only;
                (start + offset)->update_time = order.update_time;
                long version_number = (*(start + offset)).version_number;
                (start + offset)->version_number = version_number + 1;

                if (hist_update_time > 0) {
                    update_result = 1;
                }
            }
            common_release_lock(&((start + offset)->lock));

            return update_result;
        } else {
            return 0;
        }
    }

    std::shared_ptr<OrderShm> order_shm_reader_get(OrderShm* start, int offset) {
        std::shared_ptr<OrderShm> shared_instance(new OrderShm());
        common_acquire_lock(&((start + offset)->lock));

        // return a new instance
        memcpy((*shared_instance).inst_id, (start + offset)->inst_id, strlen((start + offset)->inst_id) + 1);
        memcpy((*shared_instance).type, (start + offset)->type, strlen((start + offset)->type) + 1);
        memcpy((*shared_instance).side, (start + offset)->side, strlen((start + offset)->side) + 1);
        memcpy((*shared_instance).pos_side, (start + offset)->pos_side, strlen((start + offset)->pos_side) + 1);
        memcpy((*shared_instance).time_in_force, (start + offset)->time_in_force, strlen((start + offset)->time_in_force) + 1);
        memcpy((*shared_instance).client_order_id, (start + offset)->client_order_id, strlen((start + offset)->client_order_id) + 1);
        (*shared_instance).price = (start + offset)->price;
        (*shared_instance).volume = (start + offset)->volume;
        (*shared_instance).reduce_only = (start + offset)->reduce_only;
        (*shared_instance).update_time = (start + offset)->update_time;
        (*shared_instance).version_number = (start + offset)->version_number;
    
        common_release_lock(&((start + offset)->lock));

        return shared_instance;
    }

    void order_shm_reader_detach(OrderShm* start) {
        shmdt(start);
    }
}