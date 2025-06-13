#include <iostream>
#include "config/trader_config.h"
#include "trader/global_context.h"
#include "trader/service_manager.h"
#include "trader/trade_processor.h"
/*
    loop scan new order from share memory
    send order to binance server
*/
int main(int argc, char const *argv[]) {

    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " config_file" << std::endl;
        return 0;
    }

    trader::TraderConfig config;
    if (!config.loadTraderConfig(argv[1])) {
        std::cerr << "Load config error : " << argv[1] << std::endl;
        return 1;
    }

    // init logger
    spdlog::level::level_enum logger_level = static_cast<spdlog::level::level_enum>(config.logger_level);
    init_daily_file_log(config.logger_name, config.logger_file_path, logger_level, config.logger_max_files);

    trader::GlobalContext context;
    context.init(config);

    // start trading best path service management
    trader::start_best_service_management(config, context);

    // create trader thread for per asset
    trader::start_trade_processors(config, context);
    
    std::cout << "<< this is trader process >>" << std::endl;
    info_log("trader processor started.");
    
    while(true) {
        std::cout << "process trader keep running" << std::endl;
        // info_log("process keep running");
        std::this_thread::sleep_for(std::chrono::seconds(10));
    }
}
