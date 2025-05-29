#include "account_prepare.h"

namespace actuary {

    void load_account_info(ActuaryConfig &config) {
        binance::BinanceFuturesRestClient client;
        if (config.rest_local_ip.size() > 0) {
            client.setLocalIP(config.rest_local_ip);
        }
        client.init(config.api_key_hmac, config.secret_key_hmac, config.rest_use_intranet);
        binance::CommonRestResponse<binance::FuturesAccount> response;
        client.get_account_v2(response);
        std::cout << response.code << "," << response.msg << std::endl;
        for (size_t i = 0; i < response.data.assets.size(); ++i) {
            std::cout << "asset=" << response.data.assets[i].asset << ", balance=" << response.data.assets[i].walletBalance << std::endl;
        }

        binance::BinanceSpotRestClient spotClient;
        if (config.rest_local_ip.size() > 0) {
            spotClient.setLocalIP(config.rest_local_ip);
        }
        spotClient.init(config.api_key_hmac, config.secret_key_hmac, config.rest_use_intranet);
        binance::CommonRestResponse<binance::SpotAccount> spotResponse;
        spotClient.get_account(spotResponse);
        std::cout << spotResponse.code << "," << spotResponse.msg << std::endl;
        for (size_t i = 0; i < spotResponse.data.balances.size(); ++i) {
            std::cout << "asset=" << spotResponse.data.balances[i].asset << ", balance=" << spotResponse.data.balances[i].free << std::endl;
        }
    }
}