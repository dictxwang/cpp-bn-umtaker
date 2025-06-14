#include "actuary_config.h"

namespace actuary {
    bool ActuaryConfig::loadActuaryConfig(const char* inputfile) {
        bool loadFileResult = Config::load_config(inputfile);
        if (!loadFileResult) {
            return false;
        }

        // Parse own configuration properties
        this->account_flag = this->doc_["account_flag"].asString();
        this->benchmark_inst_config_file = this->doc_["benchmark_inst_config_file"].asString();
        this->follower_inst_config_file = this->doc_["follower_inst_config_file"].asString();

        this->tg_bot_token = this->doc_["tg_bot_token"].asString();
        this->tg_chat_id = this->doc_["tg_chat_id"].asInt64();
        this->tg_send_message = this->doc_["tg_send_message"].asBool();

        this->margin_ratio_thresholds[0] = this->doc_["margin_ratio_thresholds"][0].asDouble();
        this->margin_ratio_thresholds[1] = this->doc_["margin_ratio_thresholds"][1].asDouble();

        this->bnb_balance_thresholds[0] = this->doc_["bnb_balance_thresholds"][0].asDouble();
        this->bnb_balance_thresholds[1] = this->doc_["bnb_balance_thresholds"][1].asDouble();

        this->order_size_zoom = this->doc_["order_size_zoom"].asInt();
        this->max_position_zoom = this->doc_["max_position_zoom"].asInt();

        this->api_key_hmac = this->doc_["api_key_hmac"].asString();
        this-> secret_key_hmac = this->doc_["secret_key_hmac"].asString();
        this->rest_use_intranet = this->doc_["rest_use_intranet"].asBool();
        this->rest_local_ip = this->doc_["rest_local_ip"].asString();

        this->ticker_valid_millis = this->doc_["ticker_valid_millis"].asUInt64();
        this->loop_pause_time_seconds = this->doc_["loop_pause_time_seconds"].asInt64();

        this->benchmark_quote_asset = this->doc_["benchmark_quote_asset"].asString();
        this->follower_quote_asset = this->doc_["follower_quote_asset"].asString();

        for (int i = 0; i < this->doc_["base_asset_list"].size(); i++) {
            this->base_asset_list.push_back(this->doc_["base_asset_list"][i].asString());
        }

        this->process_account_settings = this->doc_["process_account_settings"].asBool();
        this->initial_leverage = this->doc_["initial_leverage"].asInt();
        this->margin_type = this->doc_["margin_type"].asString();
        this->multi_assets_margin = this->doc_["multi_assets_margin"].asBool();
        this->position_side_dual = this->doc_["position_side_dual"].asBool();

        this->share_memory_project_id = this->doc_["share_memory_project_id"].asInt();
        this->share_memory_path_benchmark_ticker = this->doc_["share_memory_path_benchmark_ticker"].asString();
        this->share_memory_path_follower_ticker = this->doc_["share_memory_path_follower_ticker"].asString();
        this->share_memory_path_early_run = this->doc_["share_memory_path_early_run"].asString();
        this->share_memory_path_benchmark_beta = this->doc_["share_memory_path_benchmark_beta"].asString();
        this->share_memory_path_follower_beta = this->doc_["share_memory_path_follower_beta"].asString();
        this->share_memory_path_order = this->doc_["share_memory_path_order"].asString();

        return true;
    }
}