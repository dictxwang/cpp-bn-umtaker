#include "receiver_config.h"

using namespace std;

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

        this->use_udp_ticker = this->doc_["use_udp_ticker"].asBool();
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

        for (int i = 0; i < this->doc_["base_asset_list"].size(); i++) {
            this->base_asset_list.push_back(this->doc_["base_asset_list"][i].asString());
        }
        
        this->share_memory_project_id = this->doc_["share_memory_project_id"].asInt();
        this->share_memory_path_benchmark_ticker = this->doc_["share_memory_path_benchmark_ticker"].asString();
        this->share_memory_path_follower_ticker = this->doc_["share_memory_path_follower_ticker"].asString();
        this->share_memory_path_early_run = this->doc_["share_memory_path_early_run"].asString();
        this->share_memory_path_beta = this->doc_["share_memory_path_beta"].asString();

        return true;
    }
}