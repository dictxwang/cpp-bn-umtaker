#include "actuary_config.h"

namespace actuary {
    bool ActuaryConfig::loadActuaryConfig(const char* inputfile) {
        bool loadFileResult = Config::load_config(inputfile);
        if (!loadFileResult) {
            return false;
        }

        // Parse own configuration properties
        this->inst_config_file = this->doc_["inst_config_file"].asString();

        this->ticker_valid_millis = this->doc_["ticker_valid_millis"].asUInt64();

        this->benchmark_quote_asset = this->doc_["benchmark_quote_asset"].asString();
        this->follower_quote_asset = this->doc_["follower_quote_asset"].asString();

        for (int i = 0; i < this->doc_["base_asset_list"].size(); i++) {
            this->base_asset_list.push_back(this->doc_["base_asset_list"][i].asString());
        }

        this->share_memory_project_id = this->doc_["share_memory_project_id"].asInt();
        this->share_memory_path_benchmark_ticker = this->doc_["share_memory_path_benchmark_ticker"].asString();
        this->share_memory_path_follower_ticker = this->doc_["share_memory_path_follower_ticker"].asString();
        this->share_memory_path_early_run = this->doc_["share_memory_path_early_run"].asString();
        this->share_memory_path_beta = this->doc_["share_memory_path_beta"].asString();
        this->share_memory_path_order = this->doc_["share_memory_path_order"].asString();

        return true;
    }
}