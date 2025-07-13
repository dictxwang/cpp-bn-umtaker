#ifndef _CONFIG_EVALUATOR_CONFIG_H_
#define _CONFIG_EVALUATOR_CONFIG_H_

#include "config.h"

namespace evaluator {
    class EvaluatorConfig : public Config
    {
    public:
        EvaluatorConfig() {}
        ~EvaluatorConfig() {}

    public:
        bool loadEvaluatorConfig(const char* inputfile);
    
    public:
        string db_host;
        unsigned int db_port;
        string db_username;
        string db_password;
        string db_database;

        std::vector<string> accounts_need_statistics;
        std::vector<string> order_stat_zmq_ipcs;

        string normal_ticker_local_ip;
    };
}
#endif