#ifndef _TRADER_BASIC_CONTAINER_H_
#define _TRADER_BASIC_CONTAINER_H_
#include "shm/order_shm.h"

namespace trader {

    struct ShmStoreInfo {

        int order_shm_id;
        shm_mng::OrderShm* order_start;
    };
}
#endif