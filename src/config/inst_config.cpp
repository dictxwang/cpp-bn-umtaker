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
            item.min_ticker_size = binance::str_to_dobule(info_list[i]["min_ticker_size"]);
            item.max_ticker_size = binance::str_to_dobule(info_list[i]["max_ticker_size"]);
            item.order_size = binance::str_to_dobule(info_list[i]["order_size"]);
            
            this->inst_map[item.asset] = item;
        }
    }
    return true;
}