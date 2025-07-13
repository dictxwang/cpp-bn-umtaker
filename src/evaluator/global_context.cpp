#include "global_context.h"

using namespace std;

namespace evaluator {

    void GlobalContext::init(EvaluatorConfig& config) {

        this->mysql_source = make_shared<db_source::MySQLConnectionPool>(
            config.db_host,
            config.db_username,
            config.db_password,
            config.db_database
        );
        info_log("finish init db source of mysql");

        this->futures_ticker_channel = make_shared<moodycamel::ConcurrentQueue<std::string>>();
    }

    shared_ptr<db_source::MySQLConnectionPool> GlobalContext::get_mysql_source() {
        return this->mysql_source;
    }
    shared_ptr<moodycamel::ConcurrentQueue<std::string>> GlobalContext::get_futures_ticker_channel() {
        return this->futures_ticker_channel;
    }

    TickerLiteComposite& GlobalContext::get_futures_ticker_composite() {
        return this->futures_ticker_composite;
    }

    StatOrderFullInfoComposite& GlobalContext::get_stat_order_composite() {
        return this->stat_order_composite;
    }
}