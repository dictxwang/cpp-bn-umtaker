#include <iostream>
#include "config/evaluator_config.h"
#include "evaluator/global_context.h"
#include "evaluator/ticker_subscribe.h"
#include "evaluator/zmq_subscribe.h"
#include "evaluator/order_stat_manager.h"

/*
    receive order trading event, and calculate expected pnl
*/
int main(int argc, char const *argv[]) {

    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " config_file" << std::endl;
        return 0;
    }

    evaluator::EvaluatorConfig config;
    if (!config.loadEvaluatorConfig(argv[1])) {
        std::cerr << "Load config error : " << argv[1] << std::endl;
        return 1;
    }

    // init logger
    spdlog::level::level_enum logger_level = static_cast<spdlog::level::level_enum>(config.logger_level);
    init_daily_file_log(config.logger_name, config.logger_file_path, logger_level, config.logger_max_files);

    evaluator::GlobalContext context;
    context.init(config);

    // subscribe futures ticker
    evaluator::start_subscribe_normal_ticker(config, context);

    // subscribe zmq to receive stat order info
    evaluator::start_order_stat_zmq(config, context);

    // start order stat manager
    evaluator::start_order_stat_manager(config, context);

    std::cout << "<< this is evaluator process >>" << std::endl;
    info_log("evaluator processor started.");

    while(true) {
        std::cout << "process evaluator keep running" << std::endl;
        // info_log("process keep running");
        std::this_thread::sleep_for(std::chrono::seconds(10));
    }
}