#include "global_context.h"

namespace receiver {
    
    void GlobalContext::init(ReceiverConfig& config) {
        this -> benchmark_ticker_composite.init(config.follower_quote_asset, config.base_asset_list, config.normal_ticker_use_intranet);
        this -> follower_ticker_composite.init(config.follower_quote_asset, config.base_asset_list, config.normal_ticker_use_intranet);

        for (string base : config.base_asset_list) {
            string benchmark_inst = base + config.benchmark_quote_asset;
            string follower_inst = base + config.follower_quote_asset;
            this->benchmark_inst_ids.push_back(benchmark_inst);
            this->follower_inst_ids.push_back(follower_inst);
            this->inst_ids_set.insert(benchmark_inst);
            this->inst_ids_set.insert(follower_inst);
        }
    };

    TickerComposite& GlobalContext::get_benchmark_ticker_composite() {
        return this->benchmark_ticker_composite;
    }

    TickerComposite& GlobalContext::get_follower_ticker_composite() {
        return this->follower_ticker_composite;
    }

    vector<string>& GlobalContext::get_benchmark_inst_ids() {
        return this->benchmark_inst_ids;
    }

    vector<string>& GlobalContext::get_follower_inst_ids() {
        return this->follower_inst_ids;
    }

    set<string>& GlobalContext::get_inst_ids_set() {
        return this->inst_ids_set;
    }
}