#include "service_manager.h"

namespace trader {
    
    void start_service_management(TraderConfig &config, GlobalContext &context) {

    }
    void start_service_zookeeper(TraderConfig &config, GlobalContext &context) {}

    void polling_best_path_processor(TraderConfig &config, GlobalContext &context) {

        this_thread::sleep_for(chrono::seconds(5));
        while (true) {
            // TODO
        }
    }
    
    void subscribe_best_path_processor(TraderConfig &config, GlobalContext &context) {}
}