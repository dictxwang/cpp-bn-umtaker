#ifndef _CONFIG_RECEIVER_CONFIG_H_
#define _CONFIG_RECEIVER_CONFIG_H_

#include "config.h"
#include "common/shm_udp_ticker.container.h"

namespace receiver {

    class ReceiverConfig : public Config
    {
    public:
        ReceiverConfig() {}
        ~ReceiverConfig() {}

    public:
        bool loadReceiverConfig(const char* inputfile);

    public:
        string benchmark_inst_config_file;
        string follower_inst_config_file;

        string use_ticker_source; // normal/zmq/udp

        bool normal_ticker_use_intranet;
        string normal_ticker_local_ip;
        std::vector<string> ticker_zmq_ipcs;
        std::vector<UDPTickerIPC> ticker_udp_ipcs;

        uint64_t calculate_sma_interval_seconds;
        uint64_t stats_interval_seconds;

        string benchmark_quote_asset;
        string follower_quote_asset;

        bool group_main_node; // only main node should init share memory
        std::vector<string> node_base_assets;
        std::vector<string> all_base_assets; // all assets which support taker strategy

        bool enable_beta_strategy;
        bool enable_early_run_strategy;

        string early_run_calculation_type;

        int share_memory_project_id; // a single character or integer for the project identifier.
        string share_memory_path_benchmark_ticker; // make sure the path is exists, and accessiable
        string share_memory_path_follower_ticker;
        string share_memory_path_early_run;
        string share_memory_path_benchmark_beta;
        string share_memory_path_follower_beta;
    };
}
#endif  