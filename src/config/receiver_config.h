#ifndef _CONFIG_RECEIVER_CONFIG_H_
#define _CONFIG_RECEIVER_CONFIG_H_

#include "config.h"

namespace receiver {
    class ReceiverConfig : public Config
    {
    public:
        ReceiverConfig() {}
        ~ReceiverConfig() {}

    public:
        bool loadReceiverConfig(const char* inputfile);

    public:
        string inst_config_file;
        bool use_best_ticker;
        std::vector<string> ticker_zmq_ipcs;
        bool use_normal_ticker;
        bool normal_ticker_use_intranet;
        string normal_ticker_local_ip;

        uint64_t calculate_sma_interval_seconds;
        uint64_t stats_interval_seconds;

        string benchmark_quote_asset;
        string follower_quote_asset;
        std::vector<string> base_asset_list;

        bool enable_beta_strategy;
        bool enable_early_run_strategy;

        int share_memory_project_id; // a single character or integer for the project identifier.
        string share_memory_path_ticker; // make sure the path is exists, and accessiable
        string share_memory_path_early_run;
        string share_memory_path_beta;
    };
}
#endif  