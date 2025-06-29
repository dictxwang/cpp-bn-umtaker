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
        string db_host;
        unsigned int db_port;
        string db_username;
        string db_password;
        string db_database;

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

        bool enable_benchmark_ticker_trigger;
        bool enable_follower_ticker_trigger;
        uint64_t benchmark_ticker_validity_millis;
        uint64_t follower_ticker_validity_millis;
        uint64_t threshold_validity_millis;
        long loop_pause_time_millis;
        double order_price_margin;
        bool enable_position_threshold;
        bool enable_order_reduce_only;
        long same_price_pause_time_millis;
        bool enable_write_parameter_log;

        string benchmark_quote_asset;
        string follower_quote_asset;

        bool shm_group_main_node; // only main node of group should init share memory
        bool dt_group_main_node; // only main node of group should do data stat
        std::vector<string> node_base_assets;
        std::vector<string> all_base_assets; // all assets which support taker strategy
        
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