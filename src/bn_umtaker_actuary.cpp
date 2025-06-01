#include <iostream>
#include "config/actuary_config.h"
#include "actuary/global_context.h"
#include "actuary/strategy_processor.h"
#include "actuary/account_prepare.h"

/*
    loop scan ticker and threshold like 'early-run' 'beta' from share memory
    calculate the opportunities and make order, and save order to share memory
*/
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

    // account prepare
    actuary::process_account_settings(config, context);
    
    actuary::load_account_info(config);

    // create threads for per asset
    actuary::start_strategy_processors(config, context);

    std::cout << "<< this is acturay process >>" << std::endl;
    info_log("acturay processor started.");
    
    while(true) {
        std::cout << "process actuary keep running" << std::endl;
        // info_log("process keep running");
        std::this_thread::sleep_for(std::chrono::seconds(10));
    }
}
