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
        string account_flag;
        string benchmark_inst_config_file;
        string follower_inst_config_file;

        string tg_bot_token;
        int64_t tg_chat_id;
        bool tg_send_message;

        double margin_ratio_thresholds[2];
        double bnb_balance_thresholds[2];
        int order_size_zoom;
        int max_position_zoom;
        
        string api_key_hmac;
        string secret_key_hmac;
        bool rest_use_intranet;
        string rest_local_ip;

        uint64_t ticker_valid_millis;
        long loop_pause_time_millis;
        double order_price_margin;

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
        string share_memory_path_benchmark_beta;
        string share_memory_path_follower_beta;
        string share_memory_path_order;
    };
}
#endif