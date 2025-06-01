#include "account_processor.h"

namespace actuary {

    void prepare_account_settings(ActuaryConfig &config, GlobalContext &context) {
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

    void start_polling_load_balance_position(ActuaryConfig &config, GlobalContext &context) {
        std::thread polling_thread(load_balance_position, std::ref(config), std::ref(context));
        polling_thread.detach();
    }

    void load_balance_position(ActuaryConfig &config, GlobalContext &context) {
        while (true) {
            binance::CommonRestResponse<binance::FuturesAccount> response;
            context.get_furures_rest_client().get_account_v2(response);
            
            if (response.code != binance::RestCodeOK) {
                err_log("fail to load account info: {} {}", response.code, response.msg);
            } else {
                context.get_balance_position_composite().update_meta(response.data);
                info_log("update account meta: multiAssetsMargin={} totalInitialMargin={} totalMaintMargin={} \
                    totalWalletBalance={} totalUnrealizedProfit={} totalCrossWalletBalance={} totalCrossUnPnl={}",
                    response.data.multiAssetsMargin, response.data.totalInitialMargin, response.data.totalMaintMargin,
                    response.data.totalWalletBalance, response.data.totalUnrealizedProfit, response.data.totalCrossWalletBalance,
                    response.data.totalCrossUnPnl
                );

                bool updated = false;
                if (response.data.assets.size() > 0) {
                    for (size_t i = 0; i <= response.data.assets.size(); ++i) {
                        updated = context.get_balance_position_composite().update_exist_balance(response.data.assets[i]);
                        if (updated) {
                            info_log("update account balance: asset={} walletBalance={} crossWalletBalance={} crossUnPnl={}",
                                response.data.assets[i].asset, response.data.assets[i].walletBalance,
                                response.data.assets[i].crossWalletBalance, response.data.assets[i].crossUnPnl
                            );
                        }
                    }
                }
                if (response.data.positions.size() > 0) {
                    for (size_t i = 0; i <= response.data.positions.size(); ++i) {
                        updated = context.get_balance_position_composite().update_exist_position(response.data.positions[i]);
                        if (updated) {
                            info_log("update account position: symbol={} initialMargin={} maintMargin={} unrealizedProfit={} \
                                entryPrice={} positionSide={} positionAmt={}",
                                response.data.positions[i].symbol, response.data.positions[i].initialMargin,
                                response.data.positions[i].maintMargin, response.data.positions[i].unrealizedProfit,
                                response.data.positions[i].entryPrice, response.data.positions[i].positionSide,
                                response.data.positions[i].positionAmt
                            );
                        }
                    }
                }
            }
            std::this_thread::sleep_for(std::chrono::seconds(10));
        }
    }

    void start_subscribe_balance_position(ActuaryConfig &config, GlobalContext &context) {

        std::thread listen_key_thread(refresh_listen_key, std::ref(config), std::ref(context));
        listen_key_thread.detach();

        std::thread subscribe_thread(subscribe_balance_position, std::ref(config), std::ref(context));
        subscribe_thread.detach();

        std::thread process_thread(process_balance_position, std::ref(config), std::ref(context));
        process_thread.detach();
    }

    void refresh_listen_key(ActuaryConfig &config, GlobalContext &context) {

        while (true) {
            string original = context.get_listen_key();
            if (original.size() == 0) {
                // Get new listenKey
                binance::CommonRestResponse<std::string> response;
                context.get_furures_rest_client().start_userDataStream(response);
                if (response.code != binance::RestCodeOK) {
                    err_log("fail to get user listenKey {} {}", response.code, response.msg);
                    std::this_thread::sleep_for(std::chrono::seconds(5));
                    continue;
                } else {
                    context.set_listen_key(response.data);
                    info_log("get new listen key: {}", response.data);
                }
            } else {
                // Refresh listenKey
                binance::CommonRestResponse<std::string> response;
                context.get_furures_rest_client().keep_userDataStream(original, response);
                if (response.code != binance::RestCodeOK) {
                    err_log("fail to refresh user listenKey {} {}", response.code, response.msg);
                    context.set_listen_key(""); // clear original listen key
                    std::this_thread::sleep_for(std::chrono::seconds(5));
                    continue;
                } else {
                    info_log("keep original listen key: {}", original);
                }
            }

            std::this_thread::sleep_for(std::chrono::minutes(5));
        }
    }

    void subscribe_balance_position(ActuaryConfig &config, GlobalContext &context) {
        
        binance::BinanceFuturesWsClient futuresWsClient;
        if (config.rest_local_ip.size() > 0) {
            futuresWsClient.setLocalIP(config.rest_local_ip);
        }
        futuresWsClient.initUserDataStreamV1(config.api_key_hmac, config.secret_key_hmac, config.rest_use_intranet);
        futuresWsClient.setMessageChannel(context.get_account_info_channel());

        while (true) {
            string listen_key = context.get_listen_key();
            if (listen_key.size() > 0) {
                std::pair<bool, string> result;
                try {
                    result = futuresWsClient.startUserDataStreamV1(listen_key);
                } catch (std::exception &exp) {
                    err_log("error occur while start user data stream v1: {}", std::string(exp.what()));
                }

                err_log("stop user data stream v1 subscribe: {}", result.second);
            }

            // wait for a while after exception
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
    }

    void process_balance_position(ActuaryConfig &config, GlobalContext &context) {
        // TODO
    }
}