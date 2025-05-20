#include "receiver_config.h"

bool ReceiverConfig::loadReceiverConfig(const char* inputfile) {
    bool loadFileResult = Config::load_config(inputfile);
    if (!loadFileResult) {
        return false;
    }

    // Parse own configuration properties
    for (int i = 0; i < this->doc_["ticker_zmq_ipcs"].size(); i++) {
        this->ticker_zmq_ipcs.push_back(this->doc_["ticker_zmq_ipcs"][i].asString());
    }

    this->use_best_ticker = this->doc_["use_best_ticker"].asBool();
    this->use_normal_ticker = this->doc_["use_normal_ticker"].asBool();
    this->normal_ticker_use_intranet = this->doc_["normal_ticker_use_intranet"].asBool();
    this->normal_ticker_local_ip = this->doc_["normal_ticker_local_ip"].asString();

    this->ticker_validity_period_seconds = this->doc_["ticker_validity_period_seconds"].asUInt64();

    this->benchmark_quote_asset = this->doc_["benchmark_quote_asset"].asString();
    this->follower_quote_asset = this->doc_["follower_quote_asset"].asString();

    for (int i = 0; i < this->doc_["base_asset_list"].size(); i++) {
        this->base_asset_list.push_back(this->doc_["base_asset_list"][i].asString());
    }

    return true;
}