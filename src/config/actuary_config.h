#ifndef _CONFIG_ACTUARY_CONFIG_H_
#define _CONFIG_ACTUARY_CONFIG_H_

#include "config.h"

namespace actuary {
    class ActuaryConfig : public Config
    {
    public:
        ActuaryConfig() {}
        ~ActuaryConfig() {}

    public:
        bool loadActuaryConfig(const char* inputfile);
    
    public:
        string inst_config_file;
        
        string api_key_hmac;
        string secret_key_hmac;
        bool rest_use_intranet;
        string rest_local_ip;

        uint64_t ticker_valid_millis;
        long loop_pause_time_seconds;

        string benchmark_quote_asset;
        string follower_quote_asset;
        std::vector<string> base_asset_list;
        
        bool process_account_settings;
        int initial_leverage;
        string margin_type;
        bool multi_assets_margin;
        bool position_side_dual;

        int share_memory_project_id; // a single character or integer for the project identifier.
        string share_memory_path_benchmark_ticker; // make sure the path is exists, and accessiable
        string share_memory_path_follower_ticker;
        string share_memory_path_early_run;
        string share_memory_path_beta;
        string share_memory_path_order;
    };
}
#endif