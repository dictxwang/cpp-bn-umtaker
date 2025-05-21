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
        bool use_best_ticker;
        std::vector<string> ticker_zmq_ipcs;
        bool use_normal_ticker;
        bool normal_ticker_use_intranet;
        string normal_ticker_local_ip;

        uint64_t ticker_validity_period_seconds;

        string benchmark_quote_asset;
        string follower_quote_asset;
        std::vector<string> base_asset_list;
    };
}
#endif  