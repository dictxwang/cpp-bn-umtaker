#ifndef _TRADER_PROCESSOR_H_
#define _TRADER_PROCESSOR_H_

#include "config/trader_config.h"
#include "global_context.h"

namespace trader {

    void start_trade_processors(TraderConfig& config, GlobalContext& context);
    void scan_and_send_order(TraderConfig& config, GlobalContext& context, std::string& base_assets);
    
    void start_order_service(TraderConfig& config, GlobalContext& context);
    void process_order_message(TraderConfig& config, GlobalContext& context);
} 

#endif