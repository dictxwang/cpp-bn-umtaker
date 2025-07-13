#ifndef _EVALUATOR_GLOBAL_CONTEXT_H_
#define _EVALUATOR_GLOBAL_CONTEXT_H_

#include <memory>
#include "config/evaluator_config.h"
#include "order_container.h"
#include "ticker_container.h"
#include "logger/logger.h"
#include "dbsource/mysql_source.h"
#include "binancecpp/moodycamel/concurrentqueue.h"

using namespace std;

namespace evaluator {

    class GlobalContext {
    public:
        GlobalContext() {};
        ~GlobalContext() {};
        
    private:
        TickerLiteComposite futures_ticker_composite;
        StatOrderFullInfoComposite stat_order_composite;
        shared_ptr<db_source::MySQLConnectionPool> mysql_source;
        shared_ptr<moodycamel::ConcurrentQueue<std::string>> futures_ticker_channel;

    public:
        void init(EvaluatorConfig& config);
        shared_ptr<db_source::MySQLConnectionPool> get_mysql_source();
        TickerLiteComposite& get_futures_ticker_composite();
        StatOrderFullInfoComposite& get_stat_order_composite();
        shared_ptr<moodycamel::ConcurrentQueue<std::string>> get_futures_ticker_channel();
    };
}
#endif