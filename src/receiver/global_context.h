#ifndef _RECEIVER_GLOBAL_CONTEXT_H_
#define _RECEIVER_GLOBAL_CONTEXT_H_

#include "ticker_container.h"
#include "config/receiver_config.h"
#include <set>

namespace receiver {

    class GlobalContext {
    public:
        GlobalContext() {};
        ~GlobalContext() {};
    
    private:
        vector<string> benchmark_inst_ids;
        vector<string> follower_inst_ids;
        set<string> inst_ids_set;

        TickerComposite benchmark_ticker_composite;
        TickerComposite follower_ticker_composite;
    
    public:
        void init(ReceiverConfig& config);
        TickerComposite& get_benchmark_ticker_composite();
        TickerComposite& get_follower_ticker_composite();
        vector<string>& get_benchmark_inst_ids();
        vector<string>& get_follower_inst_ids();
        set<string>& get_inst_ids_set();
    };
}

#endif