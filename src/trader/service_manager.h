#ifndef _TRADER_SERVICE_MANAGER_H_
#define _TRADER_SERVICE_MANAGER_H_

#include "binancecpp/util/string_helper.h"
#include "service_container.h"
#include "global_context.h"
#include "zmq/zmq_client.h"
#include <curl/curl.h>
#include <stdexcept>

using namespace std;

namespace trader {

    const string BEST_PATH_ACTION_NEW = "NEW";
    const string BEST_PATH_ACTION_CANCEL = "CANCEL";

    struct BestPathInfo {
        string symbol;
        string local_ip;
        string remote_ip;
        string action;
        uint64_t best_cost;
        uint64_t update_time_millis = 0;
    };

    static size_t curl_write_callback(void* contents, size_t size, size_t nmemb, string* userData) {
        size_t totalSize = size * nmemb;
        userData->append(static_cast<char*>(contents), totalSize);
        return totalSize;
    }
    
    void start_best_service_management(TraderConfig &config, GlobalContext &context);

    void service_zookeeper_processor(TraderConfig &config, GlobalContext &context); // interval checking service alive status and clear unused services
    void polling_best_path_processor(TraderConfig &config, GlobalContext &context);
    void subscribe_best_path_processor(TraderConfig &config, GlobalContext &context, size_t zmq_ipc_index);

    bool is_valid_best_path(GlobalContext &context, const BestPathInfo &info);
    vector<BestPathInfo> simple_call_best_path(TraderConfig &config);
}

#endif
