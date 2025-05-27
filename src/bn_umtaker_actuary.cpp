#include <iostream>
#include "config/actuary_config.h"
#include "actuary/global_context.h"
#include "actuary/strategy_processor.h"

int main(int argc, char const *argv[]) {

    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " config_file" << std::endl;
        return 0;
    }

    actuary::ActuaryConfig config;
    if (!config.loadActuaryConfig(argv[1])) {
        std::cerr << "Load config error : " << argv[1] << std::endl;
        return 1;
    }

    // init logger
    spdlog::level::level_enum logger_level = static_cast<spdlog::level::level_enum>(config.logger_level);
    init_daily_file_log(config.logger_name, config.logger_file_path, logger_level, config.logger_max_files);

    actuary::GlobalContext context;
    context.init(config);

    // create threads for per asset
    actuary::start_strategy_processors(config, context);

    std::cout << "<< this is acturay process >>" << std::endl;
    info_log("acturay processor started.");
    
    while(true) {
        // std::cout << "Keep Running..." << std::endl;
        // info_log("Process Keep Running...");
        std::this_thread::sleep_for(std::chrono::seconds(10));
    }
}
