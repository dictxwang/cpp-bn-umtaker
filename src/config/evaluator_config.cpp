#include "evaluator_config.h"

namespace evaluator {

    bool EvaluatorConfig::loadEvaluatorConfig(const char* inputfile) {

        bool loadFileResult = Config::load_config(inputfile);
        if (!loadFileResult) {
            return false;
        }

        // Parse own configuration properties
        this->db_host = this->doc_["db_host"].asString();
        this->db_port = this->doc_["db_port"].asUInt();
        this->db_username = this->doc_["db_username"].asString();
        this->db_password = this->doc_["db_password"].asString();
        this->db_database = this->doc_["db_database"].asString();

        for (int i = 0; i < this->doc_["order_stat_zmq_ipcs"].size(); i++) {
            this->order_stat_zmq_ipcs.push_back(this->doc_["order_stat_zmq_ipcs"][i].asString());
        }
        for (int i = 0; i < this->doc_["accounts_need_statistics"].size(); i++) {
            this->accounts_need_statistics.push_back(this->doc_["accounts_need_statistics"][i].asString());
        }

        this->normal_ticker_local_ip = this->doc_["normal_ticker_local_ip"].asString();

        return true;
    }
}