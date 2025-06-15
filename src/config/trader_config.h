#ifndef _CONFIG_ACTUARY_CONFIG_H_
#define _CONFIG_ACTUARY_CONFIG_H_

#include "config.h"

namespace trader {

    class TraderConfig : public Config
    {
    public:
        TraderConfig() {}
        ~TraderConfig() {}

    public:
        bool loadTraderConfig(const char* inputfile);
    
    public:

        string api_key_ed25519;
        string secret_key_ed25519;
        bool trade_use_intranet;
        string trade_local_ip;

        bool open_place_order;
        uint64_t order_valid_millis;
        long loop_pause_time_millis;

        int order_limit_per_minute;
        int order_limit_per_10seconds;

        string benchmark_quote_asset;
        string follower_quote_asset;
        std::vector<string> base_asset_list;

        int share_memory_project_id; // a single character or integer for the project identifier.
        string share_memory_path_order; // make sure the path is exists, and accessiable

        string best_path_rest_url;
        string best_path_zmq_ipc;
        bool trading_use_best_path;
    };
}

#endif