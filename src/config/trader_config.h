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

        string benchmark_quote_asset;
        string follower_quote_asset;
        std::vector<string> base_asset_list;

        int share_memory_project_id; // a single character or integer for the project identifier.
        string share_memory_path_order; // make sure the path is exists, and accessiable
    };
}

#endif