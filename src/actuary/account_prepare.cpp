#include "account_prepare.h"

namespace actuary {

    void process_account_settings(ActuaryConfig &config, GlobalContext &context) {
        if (!config.process_account_settings) {
            info_log("no need process account settings.");
            return;
        }

        if (!toggle_bnb_burn(config, context)) {
            exit(-1);
        }
        if (!change_init_leverage(config, context)) {
            exit(-1);
        }
        if (!change_margin_type(config, context)) {
            exit(-1);
        }
        if (!change_multi_assets_margin(config, context)) {
            exit(-1);
        }
        if (!change_position_side_dual(config, context)) {
            exit(-1);
        }
    }

    bool toggle_bnb_burn(ActuaryConfig &config, GlobalContext &context) {
        binance::CommonRestResponse<bool> response;
        context.get_furures_rest_client().toggle_bnbFeeBurn(true, response);
        if (response.code != binance::RestCodeOK) {
            err_log("fail to toggle bnbFeeBurn: {} {}", response.code, response.msg);
            return false;
        } else {
            return response.data;
        }
    }

    bool change_init_leverage(ActuaryConfig &config, GlobalContext &context) {
        for (std::string follower_inst : context.get_follower_inst_ids()) {
            binance::CommonRestResponse<binance::FuturesChangeLeverageResult> response;
            context.get_furures_rest_client().change_initialLeverage(follower_inst, config.initial_leverage, response);
            if (response.code != binance::RestCodeOK) {
                err_log("fail to change initial leverage for {}", follower_inst);
                return false;
            }
        }
        return true;
    }

    bool change_margin_type(ActuaryConfig &config, GlobalContext &context) {for (std::string follower_inst : context.get_follower_inst_ids()) {
            binance::CommonRestResponse<bool> response;
            context.get_furures_rest_client().change_marginType(follower_inst, config.margin_type, response);
            if (response.code != binance::RestCodeOK || !response.data) {
                err_log("fail to change margin type for {}", follower_inst);
                return false;
            }
        }
        return true;
    }

    bool change_multi_assets_margin(ActuaryConfig &config, GlobalContext &context) {
        binance::CommonRestResponse<bool> response;
        context.get_furures_rest_client().change_multiAssetsMargin(config.multi_assets_margin, response);
         if (response.code != binance::RestCodeOK || !response.data) {
            err_log("fail to change multi assets margin");
            return false;
        }
        return true;
    }

    bool change_position_side_dual(ActuaryConfig &config, GlobalContext &context) {
        binance::CommonRestResponse<bool> response;
        context.get_furures_rest_client().change_positionSideDual(config.position_side_dual, response);
         if (response.code != binance::RestCodeOK || !response.data) {
            err_log("fail to change position side daul");
            return false;
        }
        return true;
    }

    void load_balabce_position(ActuaryConfig &config, GlobalContext &context) {

    }

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
            std::cout << "um asset=" << response.data.assets[i].asset << ", balance=" << response.data.assets[i].walletBalance << std::endl;
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
            std::cout << "spot asset=" << spotResponse.data.balances[i].asset << ", balance=" << spotResponse.data.balances[i].free << std::endl;
        }
    }
}