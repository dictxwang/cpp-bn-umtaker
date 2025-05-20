#include <iostream>

#include "config/receiver_config.h"
#include "logger/logger.h"

/*
    receive ticker from best ticker
    calculae beta parameter, and store into share-memory
*/
int main(int argc, char const *argv[]) {
    
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " config_file" << std::endl;
        return 0;
    }

    ReceiverConfig config;
    if (!config.loadReceiverConfig(argv[1])) {
        std::cerr << "Load config error : " << argv[1] << std::endl;
        return 1;
    }

    // init logger
    spdlog::level::level_enum logger_level = static_cast<spdlog::level::level_enum>(config.logger_level);
    init_daily_file_log(config.logger_name, config.logger_file_path, logger_level, config.logger_max_files);

    // subscribe ticker message
    if (config.use_normal_ticker) {
        
    } else if (config.use_best_ticker) {

    }

    std::cout << "<< this is main process >>" << std::endl;

    return 0;
}
