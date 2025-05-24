#include "receiver_config.h"

namespace receiver {
    bool ReceiverConfig::loadReceiverConfig(const char* inputfile) {
        bool loadFileResult = Config::load_config(inputfile);
        if (!loadFileResult) {
            return false;
        }

        // Parse own configuration properties
        this->inst_config_file = this->doc_["inst_config_file"].asString();

        for (int i = 0; i < this->doc_["ticker_zmq_ipcs"].size(); i++) {
            this->ticker_zmq_ipcs.push_back(this->doc_["ticker_zmq_ipcs"][i].asString());
        }

        this->use_best_ticker = this->doc_["use_best_ticker"].asBool();
        this->use_normal_ticker = this->doc_["use_normal_ticker"].asBool();
        this->normal_ticker_use_intranet = this->doc_["normal_ticker_use_intranet"].asBool();
        this->normal_ticker_local_ip = this->doc_["normal_ticker_local_ip"].asString();

        this->calculate_sma_interval_seconds = this->doc_["calculate_sma_interval_seconds"].asUInt64();
        this->stats_interval_seconds = this->doc_["stats_interval_seconds"].asUInt64();

        this->benchmark_quote_asset = this->doc_["benchmark_quote_asset"].asString();
        this->follower_quote_asset = this->doc_["follower_quote_asset"].asString();

        for (int i = 0; i < this->doc_["base_asset_list"].size(); i++) {
            this->base_asset_list.push_back(this->doc_["base_asset_list"][i].asString());
        }

        return true;
    }
}