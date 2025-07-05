#ifndef _CONFIG_INST_CONFIG_H_
#define _CONFIG_INST_CONFIG_H_

#include <unordered_map>
#include "config.h"
#include "binancecpp/util/binance_tool.h"

struct InstConfigItem {
    std::string asset = "";
    double volatility_a = 0;
    double volatility_b = 0;
    double volatility_c = 0;
    double beta = 0;
    double min_ticker_notional = 1000;
    double min_ticker_notional_multiple = 10;
    double order_size = 0;
    double max_position = 0;
    double position_adjust_step_ratio = 0;
    double position_adjust_step_notional = 0;
};

class InstConfig : public BaseConfig
{
    public:
        InstConfig() {}
        ~InstConfig() {}

    public:
        bool loadInstConfig(std::string& inputfile);
    
    public:
        std::unordered_map<string, InstConfigItem> inst_map;
};
#endif