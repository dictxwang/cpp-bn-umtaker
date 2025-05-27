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

        this->benchmark_quote_asset = this->doc_["benchmark_quote_asset"].asString();
        this->follower_quote_asset = this->doc_["follower_quote_asset"].asString();

        for (int i = 0; i < this->doc_["base_asset_list"].size(); i++) {
            this->base_asset_list.push_back(this->doc_["base_asset_list"][i].asString());
        }

        this->share_memory_project_id = this->doc_["share_memory_project_id"].asInt();
        this->share_memory_path_order = this->doc_["share_memory_path_order"].asString();
        return true;
    }
}