#ifndef _RECEIVER_GLOBAL_CONTEXT_H_
#define _RECEIVER_GLOBAL_CONTEXT_H_

#include "ticker_container.h"
#include "offset_container.h"
#include "common/threshold_container.h"
#include "config/receiver_config.h"
#include "config/inst_config.h"
#include "binancecpp/moodycamel/concurrentqueue.h"
#include <set>

namespace receiver {

    class GlobalContext {
    public:
        GlobalContext() {};
        ~GlobalContext() {};
    
    private:
        InstConfig inst_config;
        vector<string> benchmark_inst_ids;
        vector<string> follower_inst_ids;
        set<string> inst_ids_set;

        TickerComposite benchmark_ticker_composite;
        TickerComposite follower_ticker_composite;

        moodycamel::ConcurrentQueue<string> benchmark_ticker_channel;
        moodycamel::ConcurrentQueue<string> follower_ticker_channel;

        moodycamel::ConcurrentQueue<UmTickerInfo> ticker_info_channel;
        moodycamel::ConcurrentQueue<std::string> early_run_calculation_channel;
        moodycamel::ConcurrentQueue<std::string> beta_calculation_channel;

        PriceOffsetComposite price_offset_composite;
        EarlyRunThresholdComposite early_run_threshold_composite;
        BetaThresholdComposite beta_threshold_composite;

    
    public:
        void init(ReceiverConfig& config);
        InstConfig &get_inst_config();
        TickerComposite& get_benchmark_ticker_composite();
        TickerComposite& get_follower_ticker_composite();
        PriceOffsetComposite& get_price_offset_composite();
        EarlyRunThresholdComposite& get_early_run_threshold_composite();
        BetaThresholdComposite& get_beta_threshold_composite();
        moodycamel::ConcurrentQueue<string> *get_benchmark_ticker_channel();
        moodycamel::ConcurrentQueue<string> *get_follower_ticker_channel();
        moodycamel::ConcurrentQueue<UmTickerInfo> *get_ticker_info_channel();
        moodycamel::ConcurrentQueue<std::string> *get_early_run_calculation_channel();
        moodycamel::ConcurrentQueue<std::string> *get_beta_calculation_channel();
        vector<string>& get_benchmark_inst_ids();
        vector<string>& get_follower_inst_ids();
        set<string>& get_inst_ids_set();
    };
}

#endif