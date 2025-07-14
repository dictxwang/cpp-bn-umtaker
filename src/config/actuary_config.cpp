#include "actuary_config.h"

namespace actuary {
    bool ActuaryConfig::loadActuaryConfig(const char* inputfile) {
        bool loadFileResult = Config::load_config(inputfile);
        if (!loadFileResult) {
            return false;
        }

        // Parse own configuration properties
        this->account_flag = this->doc_["account_flag"].asString();
        this->db_host = this->doc_["db_host"].asString();
        this->db_port = this->doc_["db_port"].asUInt();
        this->db_username = this->doc_["db_username"].asString();
        this->db_password = this->doc_["db_password"].asString();
        this->db_database = this->doc_["db_database"].asString();

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

        this->enable_benchmark_ticker_trigger = this->doc_["enable_benchmark_ticker_trigger"].asBool();        this->rest_local_ip = this->doc_["rest_local_ip"].asString();
        this->enable_follower_ticker_trigger = this->doc_["enable_follower_ticker_trigger"].asBool();
        this->benchmark_ticker_validity_millis = this->doc_["benchmark_ticker_validity_millis"].asUInt64();
        this->follower_ticker_validity_millis = this->doc_["follower_ticker_validity_millis"].asUInt64();
        this->threshold_validity_millis = this->doc_["threshold_validity_millis"].asUInt64();
        this->loop_pause_time_millis = this->doc_["loop_pause_time_millis"].asInt64();
        this->order_price_margin = this->doc_["order_price_margin"].asDouble();
        this->enable_position_threshold = this->doc_["enable_position_threshold"].asBool();
        this->position_threshold_margin = this->doc_["position_threshold_margin"].asDouble();
        this->enable_order_reduce_only = this->doc_["enable_order_reduce_only"].asBool();
        this->enable_ticker_size_compare = this->doc_["enable_ticker_size_compare"].asBool();
        this->same_price_pause_time_millis = this->doc_["same_price_pause_time_millis"].asInt64();

        this->enable_write_parameter_log = this->doc_["enable_write_parameter_log"].asBool();

        this->benchmark_quote_asset = this->doc_["benchmark_quote_asset"].asString();
        this->follower_quote_asset = this->doc_["follower_quote_asset"].asString();

        this->shm_group_main_node = this->doc_["shm_group_main_node"].asBool();
        this->dt_group_main_node = this->doc_["dt_group_main_node"].asBool();
        this->zmq_ipc_order_stat = this->doc_["zmq_ipc_order_stat"].asString();
        this->enable_load_order_to_zmq = this->doc_["enable_load_order_to_zmq"].asBool();
        this->enable_save_no_volume_order = this->doc_["enable_save_no_volume_order"].asBool();

        for (int i = 0; i < this->doc_["node_base_assets"].size(); i++) {
            this->node_base_assets.push_back(this->doc_["node_base_assets"][i].asString());
        }
        for (int i = 0; i < this->doc_["all_base_assets"].size(); i++) {
            this->all_base_assets.push_back(this->doc_["all_base_assets"][i].asString());
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