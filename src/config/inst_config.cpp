#include "inst_config.h"

bool InstConfig::loadInstConfig(std::string& inputfile) {
    bool loadFileResult = BaseConfig::load_config_file(inputfile.c_str());
    if (!loadFileResult) {
        return false;
    }

    if (this->doc_.isMember("asset_info_list")) {
        Json::Value info_list = this->doc_["asset_info_list"];
        for (int i = 0; i < info_list.size(); i++) {
            InstConfigItem item;
            item.asset = info_list[i]["asset"].asString();
            item.volatility_a = binance::str_to_dobule(info_list[i]["volatility_a"]);
            item.volatility_b = binance::str_to_dobule(info_list[i]["volatility_b"]);
            item.volatility_c = binance::str_to_dobule(info_list[i]["volatility_c"]);
            item.beta = binance::str_to_dobule(info_list[i]["beta"]);
            item.min_ticker_notional = info_list[i]["min_ticker_notional"].asDouble();
            item.min_ticker_notional_multiple = info_list[i]["min_ticker_notional_multiple"].asDouble();
            item.order_size = binance::str_to_dobule(info_list[i]["order_size"]);
            item.max_position = binance::str_to_dobule(info_list[i]["max_position"]);
            item.position_adjust_step_ratio = info_list[i]["position_adjust_step_ratio"].asDouble();
            item.position_adjust_step_notional = info_list[i]["position_adjust_step_notional"].asDouble();
            
            this->inst_map[item.asset] = item;
        }
    }
    return true;
}