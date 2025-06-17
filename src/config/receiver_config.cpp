#include "receiver_config.h"

using namespace std;

namespace receiver {
    bool ReceiverConfig::loadReceiverConfig(const char* inputfile) {
        bool loadFileResult = Config::load_config(inputfile);
        if (!loadFileResult) {
            return false;
        }

        // Parse own configuration properties
        this->benchmark_inst_config_file = this->doc_["benchmark_inst_config_file"].asString();
        this->follower_inst_config_file = this->doc_["follower_inst_config_file"].asString();

        this->use_ticker_source = this->doc_["use_ticker_source"].asString();

        this->normal_ticker_use_intranet = this->doc_["normal_ticker_use_intranet"].asBool();
        this->normal_ticker_local_ip = this->doc_["normal_ticker_local_ip"].asString();

        for (int i = 0; i < this->doc_["ticker_zmq_ipcs"].size(); i++) {
            this->ticker_zmq_ipcs.push_back(this->doc_["ticker_zmq_ipcs"][i].asString());
        }

        Json::Value json_udp_ipcs = this->doc_["ticker_udp_ipcs"];
        for (int i = 0; i < json_udp_ipcs.size(); i++) {
            UDPTickerIPC ipc;
            ipc.mmap_file = json_udp_ipcs[i]["mmap_file"].asString();
            ipc.host = json_udp_ipcs[i]["host"].asString();
            ipc.port = json_udp_ipcs[i]["port"].asInt();
            this->ticker_udp_ipcs.push_back(ipc);
        }

        this->calculate_sma_interval_seconds = this->doc_["calculate_sma_interval_seconds"].asUInt64();
        this->stats_interval_seconds = this->doc_["stats_interval_seconds"].asUInt64();

        this->benchmark_quote_asset = this->doc_["benchmark_quote_asset"].asString();
        this->follower_quote_asset = this->doc_["follower_quote_asset"].asString();

        this->group_main_node = this->doc_["group_main_node"].asBool();
        for (int i = 0; i < this->doc_["node_base_assets"].size(); i++) {
            this->node_base_assets.push_back(this->doc_["node_base_assets"][i].asString());
        }
        for (int i = 0; i < this->doc_["all_base_assets"].size(); i++) {
            this->all_base_assets.push_back(this->doc_["all_base_assets"][i].asString());
        }
        
        this->share_memory_project_id = this->doc_["share_memory_project_id"].asInt();
        this->share_memory_path_benchmark_ticker = this->doc_["share_memory_path_benchmark_ticker"].asString();
        this->share_memory_path_follower_ticker = this->doc_["share_memory_path_follower_ticker"].asString();
        this->share_memory_path_early_run = this->doc_["share_memory_path_early_run"].asString();
        this->share_memory_path_benchmark_beta = this->doc_["share_memory_path_benchmark_beta"].asString();
        this->share_memory_path_follower_beta = this->doc_["share_memory_path_follower_beta"].asString();

        return true;
    }
}