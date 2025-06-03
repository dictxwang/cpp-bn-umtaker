#include <iostream>

#include "config/receiver_config.h"
#include "receiver/global_context.h"
#include "receiver/ticker_calculate.h"
#include "receiver/ticker_subscribe.h"
#include "logger/logger.h"

/*
    receive ticker from best ticker
    calculate early run / beta parameter, and store into share-memory
*/
int main(int argc, char const *argv[]) {
    
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " config_file" << std::endl;
        return 0;
    }

    receiver::ReceiverConfig config;
    if (!config.loadReceiverConfig(argv[1])) {
        std::cerr << "Load config error : " << argv[1] << std::endl;
        return 1;
    }

    // init logger
    spdlog::level::level_enum logger_level = static_cast<spdlog::level::level_enum>(config.logger_level);
    init_daily_file_log(config.logger_name, config.logger_file_path, logger_level, config.logger_max_files);

    receiver::GlobalContext context;
    context.init(config);

    // start ticker calculate thread
    receiver::start_ticker_calculate(config, context);
    info_log("start ticker calculate.");
    
    // subscribe ticker message
    if (config.use_normal_ticker) {
        receiver::start_subscribe_normal_ticker(config, context);
        info_log("start subscribe normal ticker.");
    } else if (config.use_best_ticker) {
        receiver::start_subscribe_zmq_best_ticker(config, context);
        info_log("start subscribe best ticker.");
    } else if (config.use_udp_ticker) {
        receiver::start_subscribe_udp_ticker(config, context);
        info_log("start subscribe udp ticker.");
    }

    std::cout << "<< this is receiver process >>" << std::endl;
    info_log("receiver processor started.");
    
    while(true) {
        std::cout << "process receiver keep running" << std::endl;
        // info_log("process keep running");
        std::this_thread::sleep_for(std::chrono::seconds(10));
    }

    return 0;
}
