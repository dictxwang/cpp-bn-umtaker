#ifndef _TRADER_SERVICE_MANAGER_H_
#define _TRADER_SERVICE_MANAGER_H_

#include "service_container.h"
#include "global_context.h"

using namespace std;

namespace trader {
    
    void start_service_management(TraderConfig &config, GlobalContext &context);
    void start_service_zookeeper(TraderConfig &config, GlobalContext &context); // interval checking service alive status and clear unused services

    void polling_best_path_processor(TraderConfig &config, GlobalContext &context);
    void subscribe_best_path_processor(TraderConfig &config, GlobalContext &context);
}

#endif
