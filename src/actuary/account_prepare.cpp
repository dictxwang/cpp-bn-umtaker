#include "account_prepare.h"

namespace actuary {

    void load_account_info(ActuaryConfig &config) {
        binance::BinanceFuturesRestClient client;
        client.init(config.api_key_hmac, config.secret_key_hmac, false);
        binance::CommonRestResponse<binance::FuturesAccount> response;
        client.get_account_v2(response);
        for (size_t i = 0; i < response.data.assets.size(); ++i) {
            std::cout << "asset=" << response.data.assets[i].asset << ", balance=" << response.data.assets[i].walletBalance << std::endl;
        }
    }
}