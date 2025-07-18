#ifndef _ACTUARY_STAT_DISPATCH_H_
#define _ACTUARY_STAT_DISPATCH_H_

#include "zmq/zmq_client.h"
#include "config/actuary_config.h"
#include "global_context.h"
#include "common_container.h"
#include "dbsource/mysql_source.h"
#include "logger/logger.h"

using namespace std;

namespace actuary {

    void start_stat_zmq_server(ActuaryConfig &config, GlobalContext &context);
    void stat_zmq_server_listen(ActuaryConfig &config, GlobalContext &context);

    void start_delay_load_orders_to_zmq(ActuaryConfig &config, GlobalContext &context);
    void load_recent_orders_to_zmq(ActuaryConfig &config, GlobalContext &context);

    void start_delay_save_exchange_info(ActuaryConfig &config, GlobalContext &context);
    void delay_save_exchange_info(ActuaryConfig &config, GlobalContext &context);
}
#endif