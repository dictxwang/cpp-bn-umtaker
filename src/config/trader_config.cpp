#include "trader_config.h"

namespace trader {

    bool TraderConfig::loadTraderConfig(const char* inputfile) {
        bool loadFileResult = Config::load_config(inputfile);
        if (!loadFileResult) {
            return false;
        }

        // Parse own configuration properties
        this->api_key_ed25519 = this->doc_["api_key_ed25519"].asString();
        this->secret_key_ed25519 = this->doc_["secret_key_ed25519"].asString();
        this->trade_use_intranet = this->doc_["trade_use_intranet"].asBool();
        this->trade_local_ip = this->doc_["trade_local_ip"].asString();

        this->open_place_order = this->doc_["open_place_order"].asBool();
        this->order_valid_millis = this->doc_["order_valid_millis"].asUInt64();
        this->loop_pause_time_millis = this->doc_["loop_pause_time_millis"].asInt64();

        this->order_limit_per_minute = this->doc_["order_limit_per_minute"].asInt();
        this->order_limit_per_10seconds = this->doc_["order_limit_per_10seconds"].asInt();

        this->benchmark_quote_asset = this->doc_["benchmark_quote_asset"].asString();
        this->follower_quote_asset = this->doc_["follower_quote_asset"].asString();

        for (int i = 0; i < this->doc_["node_base_assets"].size(); i++) {
            this->node_base_assets.push_back(this->doc_["node_base_assets"][i].asString());
        }
        for (int i = 0; i < this->doc_["all_base_assets"].size(); i++) {
            this->all_base_assets.push_back(this->doc_["all_base_assets"][i].asString());
        }

        this->share_memory_project_id = this->doc_["share_memory_project_id"].asInt();
        this->share_memory_path_order = this->doc_["share_memory_path_order"].asString();

        for (int i = 0; i < this->doc_["best_path_rest_urls"].size(); i++) {
            this->best_path_rest_urls.push_back(this->doc_["best_path_rest_urls"][i].asString());
        }

        for (int i = 0; i < this->doc_["best_path_zmq_ipcs"].size(); i++) {
            this->best_path_zmq_ipcs.push_back(this->doc_["best_path_zmq_ipcs"][i].asString());
        }
        
        this->trading_use_best_path = this->doc_["trading_use_best_path"].asBool();
        return true;
    }
}