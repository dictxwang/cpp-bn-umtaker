#include <iostream>
#include <thread>
#include <chrono>
#include "binancecpp/binance.h"
#include "binancecpp/binance_param.h"
#include "binancecpp/binance_model.h"
#include "binancecpp/binance_wallet.h"
#include "binancecpp/binance_spot.h"
#include "binancecpp/binance_futures.h"
#include "binancecpp/binance_ws.h"
#include "binancecpp/binance_ws_model.h"
#include "binancecpp/binance_ws_spot.h"
#include "binancecpp/binance_ws_futures.h"
#include "binancecpp/json/json.h"
#include "config/main_config.h"
#include "logger/logger.h"
#include "zmq/zmq_client.h"
#include "common/tools.h"
#include "protocol/ticker_info.pb.h"

bool processTickerMessage(std::string &messageJson) {
    Json::Value json_result;
    Json::Reader reader;
    json_result.clear();
    reader.parse(messageJson.c_str(), json_result);

    std::cout << "process ticker message: " << json_result << std::endl;

    if (json_result.isMember("data")) {
        // combine event
        /*
        {
            "data" :
            {
                "A" : "26.85530000",
                "B" : "25.82170000",
                "a" : "1771.98000000",
                "b" : "1771.97000000",
                "s" : "ETHUSDT",
                "u" : 50463694751
            },
            "stream" : "ethusdt@bookTicker"
        }
        */
       json_result = json_result["data"];
    }
    binance::WsBookTickerEvent event = binance::convertJsonToWsBookTickerEvent(json_result);
    std::cout << "convert ticker event for : " << event.symbol << std::endl;
    return true;
}

bool processUserDataMessage(std::string &messageJson) {
    Json::Value json_result;
    Json::Reader reader;
    json_result.clear();
    reader.parse(messageJson.c_str(), json_result);

    std::cout << "process user data message: " << json_result << std::endl;

    if (json_result.isMember("event")) {
        // new version
        json_result = json_result["event"];
    }

    std::string eventType = json_result["e"].asString();
    if (eventType == "balanceUpdate") {
        binance::WsSpotBalanceUpdateEvent event = binance::convertJsonToWsSpotBalanceUpdateEvent(json_result);
        std::cout << "convert balance update event for : " << event.asset << std::endl;
    } else if (eventType == "outboundAccountPosition") {
        binance::WsSpotAccountUpdateEvent event = binance::convertJsonToWsWsSpotAccountUpdateEvent(json_result);
        std::cout << "convert account update event with size : " << event.balances.size() << std::endl;
    } else if (eventType == "executionReport") {
        binance::WsSpotOrderUpdateEvent event = binance::convertJsonToWsSpotOrderUpdateEvent(json_result);
        std::cout << "convert order update event with size : " << event.symbol << std::endl;
    }
    return true;
}

bool processFuturesTickerMessage(std::string &messageJson) {
    Json::Value json_result;
    Json::Reader reader;
    json_result.clear();
    reader.parse(messageJson.c_str(), json_result);

    std::cout << "process ticker message: " << json_result << std::endl;

    if (json_result.isMember("data")) {
        json_result = json_result["data"];
    }
    binance::WsFuturesBookTickerEvent event = binance::convertJsonToWsFuturesBookTickerEvent(json_result);
    std::cout << "convert ticker event for : " << event.symbol << std::endl;
    return true;
}

bool processMarkPriceMessage(std::string &messageJson) {
    Json::Value json_result;
    Json::Reader reader;
    json_result.clear();
    reader.parse(messageJson.c_str(), json_result);

    std::cout << "process ticker message: " << json_result << std::endl;

    if (json_result.isArray()) {
        for (int i = 0; i < json_result.size(); i++) {
            binance::WsFuturesMarkPriceEvent event = binance::convertJsonToWsFuturesMarkPriceEvent(json_result[i]);    
            std::cout << "Parse mark-price for " << event.symbol << std::endl;
        }
    } else {
        binance::WsFuturesMarkPriceEvent event = binance::convertJsonToWsFuturesMarkPriceEvent(json_result);
        std::cout << "Parse mark-price for " << event.symbol << std::endl;
    }
    return true;
}

bool processFuturesUserDataMessage(std::string &messageJson) {
    Json::Value json_result;
    Json::Reader reader;
    json_result.clear();
    reader.parse(messageJson.c_str(), json_result);

    std::cout << "process user data message: " << json_result << std::endl;
    if (json_result["e"] == binance::FuturesUserDataAccountUpdate) {
        binance::WsFuturesAccountUpdateEvent event = binance::convertJsonToWsFuturesAccountUpdateEvent(json_result);
        std::cout << "convert futures user data: accout-update: " << event.eventType << std::endl;
    } else if (json_result["e"] == binance::FuturesUserDataOrderTradeUpdate) {
        binance::WsFuturesOrderTradeUpdateEvent event = binance::convertJsonToWsFuturesOrderTradeUpdateEvent(json_result);
        std::cout << "convert futures user data: order-trade-update: " << event.eventType << std::endl;
    }
    return true;
}

bool processFuturesOrderServiceMessage(std::string &messageJson) {
    // std::cout << messageJson << std::endl;
    Json::Value json_result;
    Json::Reader reader;
    json_result.clear();
    reader.parse(messageJson.c_str(), json_result);

    std::cout << "process futures order message: " << json_result << std::endl;
    binance::WsFuturesOrderCallbackEvent event = binance::convertJsonToWsFuturesOrderCallbackEvent(json_result);
    std::cout << "status:" << event.status << std::endl;

    return true;
}

void startFuturesOrderService(binance::BinanceFuturesWsClient &futuresWsClient, std::string &apiKey, std::string &secretKey) {
    std::cout << "apiKey=" << apiKey << ",secretKey=" << secretKey << std::endl;
    try {
        // futuresWsClient.initOrderService(apiKey, secretKey, false);
        futuresWsClient.setMessageCallback(processFuturesOrderServiceMessage);
        std::pair<bool, string> startResult = futuresWsClient.startOrderService();
        std::cout << "start: " << startResult.first << "," << startResult.second << std::endl;
    } catch (std::exception &e) {
        std::cout << "futures order error: " << e.what() << std::endl;
    }
}

void startZMQSender(std::string &ipc) {
    ZMQClient zmq_client(ZMQ_PUB);
    zmq_client.PublisherBind(ipc);
    while (true) {
        besttick::TickerInfo info;
        info.set_instid("BTCUSDT");
        info.set_bestbid(1.21);
        std::string output;
        info.SerializeToString(&output);
        zmq_client.Send(output);
        // zmq_client.Send("Hello, World.");
        std::cout << "zmq send message" << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
}

void startZMQReceiver(std::string &ipc) {
    ZMQClient zmq_client(ZMQ_SUB);
    zmq_client.SubscriberConnect(ipc);
    while (true) {
        std::string message = zmq_client.Receive();
        std::cout << "zmq recive: " << message << std::endl;
        besttick::TickerInfo info;
        info.ParseFromString(message);
        std::cout << "info: " << info.instid() << "," << info.bestbid() << std::endl;
    }
}

void startMessageChannelConsume(moodycamel::ConcurrentQueue<std::string> *messageChannel) {
    while (true) {
        std::string message;
        while (!messageChannel->try_dequeue(message)) {}
        std::cout << "channel message: " << message << std::endl;
    }
}

int main(int argc, char const *argv[]) {
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " config_file" << std::endl;
        return 0;
    }

    MainConfig config;
    if (!config.loadMainConfig(argv[1])) {
        std::cerr << "Load config error : " << argv[1] << std::endl;
        return 1;
    }

    // init logger
    spdlog::level::level_enum logger_level = static_cast<spdlog::level::level_enum>(config.logger_level);
    init_daily_file_log(config.logger_name, config.logger_file_path, logger_level, config.logger_max_files);

    // Create instance of rest
    binance::BinanceWalletRestClient binanceRestWallet;
    binance::BinanceSpotRestClient binanceRestSpot;
    binance::BinanceFuturesRestClient binanceRestFutures;
    if (config.rest_local_ip.size() > 0) {
        binanceRestSpot.setLocalIP(config.rest_local_ip);
        binanceRestFutures.setLocalIP(config.rest_local_ip);
        binanceRestWallet.setLocalIP(config.rest_local_ip);
    }
    binanceRestWallet.init(config.api_key_hmac, config.secret_key_hmac, config.rest_use_intranet);
    binanceRestSpot.init(config.api_key_hmac, config.secret_key_hmac, config.rest_use_intranet);
    binanceRestFutures.init(config.api_key_hmac, config.secret_key_hmac, config.rest_use_intranet);

    // Create instance of ws
    binance::BinanceSpotWsClient binanceSpotWs;
    binance::BinanceFuturesWsClient futuresWsClient;

    // Example: setServerTimeOffset
    // binance::CommonRestResponse<uint64_t> timeResp;
    // binanceRestSpot.setServerTimeOffset(timeResp);
    // std::cout << "Response code: " << timeResp.code << std::endl;
    // std::cout << "Response data " << timeResp.data << std::endl;

    // Example: get_exchangeInfo
    // binance::CommonRestResponse<std::vector<binance::SpotExchangeInfo>> response;
    // std::vector<std::string> instIds;
    // instIds.push_back("BTCUSDT");
    // instIds.push_back("ETHUSDT");
    // binanceRestSpot.get_exchangeInfo(instIds, response);
    // std::cout << "Response code: " << response.code << std::endl;
    // std::cout << "Response symbols-0.baseAsset: " << response.data.size() << std::endl;

    // Example: get_account
    binance::CommonRestResponse<binance::SpotAccount> accountResp;
    binanceRestSpot.get_account(accountResp);
    std::cout << "Response code: " << accountResp.code << std::endl;
    std::cout << "Response msg: " << accountResp.msg << std::endl;
    std::cout << "Response data: " << accountResp.data.accountType << std::endl;
    for (binance::BalanceLite bal : accountResp.data.balances) {
        if (bal.free > 0 || bal.locked > 0) {
            std::cout << "spot balance: "<< bal.asset << "," << bal.free << std::endl;
        }
    }

    // Example: subscribe bookticker
    // std::vector<std::string> symbols;
    // symbols.push_back("BTCUSDT");
    // symbols.push_back("ETHUSDT");
    // binanceSpotWs.initBookTicker(false, false);
    // binanceSpotWs.setMessageCallback(processTickerMessage);
    // moodycamel::ConcurrentQueue<std::string> messageChannel;
    // binanceSpotWs.setMessageChannel(&messageChannel);
    // std::thread messagenChannelConsume(startMessageChannelConsume, &messageChannel);
    // messagenChannelConsume.detach();
    // std::cout << "After Inited." << std::endl;
    // binanceSpotWs.startBookTicker(symbols);

    // Example: get listenKey / refresh listenKey
    // binance::CommonRestResponse<std::string> startUserStreamResp;
    // binanceRestSpot.start_userDataStream(startUserStreamResp);
    // std::cout << "Response code: " << startUserStreamResp.code << std::endl;
    // std::cout << "Response msg: " << startUserStreamResp.msg << std::endl;
    // std::cout << "Response data-listenKey: " << startUserStreamResp.data << std::endl;

    // std::string listenKey = "2B1i7X0Y7VSyTnLcCBjxpXJTPcMPRcmZxri2L0DkRsnp6aTW4pzFeO9fvDP7";
    // binance::CommonRestResponse<std::string> keepUserStreamResp;
    // binanceRestSpot.keep_userDataStream(listenKey, keepUserStreamResp);
    // std::cout << "Response code: " << keepUserStreamResp.code << std::endl;
    // std::cout << "Response msg: " << keepUserStreamResp.msg << std::endl;

    // Example: startUserDataStreamV1
    // binanceSpotWs.initUserDataStreamV1(config.api_key_hmac, config.secret_key_hmac, false);
    // binanceSpotWs.startUserDataStreamV1(processUserDataMessage, listenKey);
    // std::cout << "end user data stream" << std::endl;

    // Example: startUserDataStream
    // binanceSpotWs.initUserDataStream(config.api_key_ed25519, config.secret_key_ed25519, false);
    // binanceSpotWs.setMessageCallback(processUserDataMessage);
    // std::pair<bool, string> startResult = binanceSpotWs.startUserDataStream();
    // std::cout << "start result: " << startResult.first << ", msg=" << startResult.second << std::endl;

    // Example: setServerTimeOffset
    // binance::CommonRestResponse<uint64_t> futuresTimeResp;
    // binanceRestFutures.setServerTimeOffset(futuresTimeResp);
    // std::cout << "Response code futures set time-offset: " << futuresTimeResp.code << std::endl;
    // std::cout << "Response data futures set time-offset:" << futuresTimeResp.data << std::endl;

    // Example: get_exchangeInfo
    // binance::CommonRestResponse<std::vector<binance::FuturesExchangeInfo>> futuresResponse;
    // binanceRestFutures.get_exchangeInfo(futuresResponse);
    // std::cout << "Response code: " << futuresResponse.code << std::endl;
    // std::cout << "Response symbols-0.symbol: " << futuresResponse.data[0].symbol << std::endl;
    // for (binance::FuturesExchangeInfo info : futuresResponse.data) {
    //     if (info.quoteAsset == "USDC") {
    //         std::cout << "symbol=" << info.symbol << ",minPrice=" << info.minPrice << ",tickSize=" << info.tickSize << ",minQty=" << info.minQty << ",maxQty=" << info.maxQty << ",stepSize=" << info.stepSize << std::endl;
    //     }
    // }

    // Example: get_account_v2
    binance::CommonRestResponse<binance::FuturesAccount> fururesAcccountResponse;
    binanceRestFutures.get_account_v2(fururesAcccountResponse);
    std::cout << "Response code: " << fururesAcccountResponse.code << std::endl;
    std::cout << "Response account canWithdraw: " << fururesAcccountResponse.data.canWithdraw << std::endl;
    std::cout << "Response account totalInitialMargin: " << fururesAcccountResponse.data.totalInitialMargin << std::endl;
    std::cout << "Response account totalCrossWalletBalance: " << fururesAcccountResponse.data.totalCrossWalletBalance << std::endl;
    for (binance::FuturesAccountAsset bal : fururesAcccountResponse.data.assets) {
        if (bal.crossWalletBalance > 0) {
            std::cout << "futures balance: " << bal.asset << "," << bal.crossWalletBalance << std::endl;
        }
    }
    // Example: get_multiAssetMargin
    // binance::CommonRestResponse<bool> futuresMultiAssetResponse;
    // binanceRestFutures.get_multiAssetMargin(futuresMultiAssetResponse);
    // std::cout << "Response-get_multiAssetMargin code: " << futuresMultiAssetResponse.code << std::endl;
    // std::cout << "Response-get_multiAssetMargin msg: " << futuresMultiAssetResponse.msg << std::endl;
    // std::cout << "Response-get_multiAssetMargin data: " << futuresMultiAssetResponse.data << std::endl;

    // Example: get_bnbFeeBurn
    // binance::CommonRestResponse<bool> futuresFeeBurnResponse;
    // binanceRestFutures.get_bnbFeeBurn(futuresFeeBurnResponse);
    // std::cout << "Response-get_bnbFeeBurn code: " << futuresMultiAssetResponse.code << std::endl;
    // std::cout << "Response-get_bnbFeeBurn msg: " << futuresMultiAssetResponse.msg << std::endl;
    // std::cout << "Response-get_bnbFeeBurn data: " << futuresMultiAssetResponse.data << std::endl;

    // Example: toggle_bnbFeeBurn
    // binance::CommonRestResponse<bool> futuresFeeBurnToggleResponse;
    // binanceRestFutures.toggle_bnbFeeBurn(true, futuresFeeBurnToggleResponse);
    // std::cout << "Response-toggle_bnbFeeBurn code: " << futuresFeeBurnToggleResponse.code << std::endl;
    // std::cout << "Response-toggle_bnbFeeBurn msg: " << futuresFeeBurnToggleResponse.msg << std::endl;
    // std::cout << "Response-toggle_bnbFeeBurn data: " << futuresFeeBurnToggleResponse.data << std::endl;

    // Example: startBookTickerV1
    // futuresWsClient.initBookTickerV1(false, false);
    // std::vector<string> futuresSymbols;
    // futuresSymbols.push_back("BTCUSDT");
    // futuresSymbols.push_back("ETHUSDT");
    // futuresWsClient.startBookTickerV1(processFuturesTickerMessage, futuresSymbols);

    // Example: startMarkPriceV1
    // futuresWsClient.initMarkPriceV1(false, false);
    // futuresWsClient.startAllMarkPricesV1(processMarkPriceMessage, binance::WsMarkPriceInterval::WsMPThreeSeconds);
    // futuresWsClient.startMarkPriceV1(processMarkPriceMessage, futuresSymbols, binance::WsMarkPriceInterval::WsMPThreeSeconds);

    // Example: start_userDataStream / keep_userDataStream
    // binance::CommonRestResponse<std::string> startFuturesUserStreamResp;
    // binanceRestFutures.start_userDataStream(startFuturesUserStreamResp);
    // std::cout << "Response code: " << startFuturesUserStreamResp.code << std::endl;
    // std::cout << "Response msg: " << startFuturesUserStreamResp.msg << std::endl;
    // std::cout << "Response data: " << startFuturesUserStreamResp.data << std::endl;
    
    // std::string futuresListenKey = "SkF9KFpQMstAbYf4KXtHD60J3nq1TYrIVYWx3oujAf2Sk2oPssRrwVZosUKGv3a3";
    // binance::CommonRestResponse<std::string> keepFuturesUserStreamResp;
    // binanceRestFutures.keep_userDataStream(futuresListenKey, keepFuturesUserStreamResp);
    // std::cout << "Response code: " << keepFuturesUserStreamResp.code << std::endl;
    // std::cout << "Response msg: " << keepFuturesUserStreamResp.msg << std::endl;

    // Example: startUserDataStreamV1
    // futuresWsClient.initUserDataStreamV1(config.api_key_hmac, config.secret_key_hmac, false);
    // futuresWsClient.startUserDataStreamV1(processFuturesUserDataMessage, futuresListenKey);

    // Example: startUserDataStream
    // futuresWsClient.initUserDataStream(config.api_key_ed25519, config.secret_key_ed25519, false);
    // futuresWsClient.startUserDataStream(processFuturesUserDataMessage);


    // Example: placeOrder / cancelOrder
    // futuresWsClient.initOrderService(config.api_key_ed25519, config.secret_key_ed25519, false);
    // std::thread futureOrderThread(startFuturesOrderService, std::ref(futuresWsClient), std::ref(config.api_key_ed25519), std::ref(config.secret_key_ed25519));
    // futureOrderThread.detach();
    // std::this_thread::sleep_for(std::chrono::seconds(2));

    // binance::FuturesNewOrder order;
    // order.symbol = "BTCUSDT";
    // order.side = binance::ORDER_SIDE_BUY;
    // order.positionSide = binance::PositionSide_LONG;
    // order.quantity = 0.002;
    // order.price = 80000.00;
    // order.type = binance::ORDER_TYPE_LIMIT;
    // order.timeInForce = binance::TimeInForce_GTC;
    // order.newClientOrderId = "a1234567890";
    // order.newOrderRespType = binance::ORDER_RESP_TYPE_RESULT;
    // try {
    //     std::pair<bool, string> placeResult = futuresWsClient.placeOrder(order);
    //     std::cout << "place result: " << placeResult.first << "," << placeResult.second << std::endl;
    // } catch (std::exception &e) {
    //     std::cout << "fail to place order: " << e.what() << std::endl;
    // }
    
    // std::this_thread::sleep_for(std::chrono::seconds(5));
    
    // binance::FuturesCancelOrder cancelOrder;
    // cancelOrder.origClientOrderId = "a1234567890";
    // cancelOrder.symbol = "BTCUSDT";
    // try {
    //     std::pair<bool, string> cancelResult = futuresWsClient.cancelOrder(cancelOrder);
    //     std::cout << "cancel result: " << cancelResult.first << "," << cancelResult.second << std::endl;
    // } catch (std::exception &e) {
    //     std::cout << "fail to cancel order: " << e.what() << std::endl;
    // }
    

    // Example: zmq sender and receiver
    // std::thread zmqSender(startZMQSender, std::ref(config.zmq_ipc));
    // std::this_thread::sleep_for(std::chrono::seconds(3));
    // std::thread zmqReceiver(startZMQReceiver, std::ref(config.zmq_ipc));

    // Example: spot order by rest
    // binance::SpotNewOrder newOrder;
    // newOrder.symbol = "USDCUSDT";
    // newOrder.side = binance::ORDER_SIDE_BUY;
    // newOrder.type = "MARKET";
    // // newOrder.type = binance::ORDER_TYPE_LIMIT;
    // // newOrder.timeInForce = binance::TimeInForce_IOC;
    // newOrder.quantity = 5000;
    // // newOrder.price = 12605;
    // newOrder.newOrderRespType = binance::ORDER_RESP_TYPE_RESULT;
    // newOrder.newClientOrderId = gen_client_order_id(true);

    // binance::CommonRestResponse<binance::SpotNewOrderResult> newOrderResp;
    // binanceRestSpot.create_new_order(newOrder, newOrderResp);
    // std::cout << "code=" << newOrderResp.code << ",msg=" << newOrderResp.msg << std::endl;
    // std::cout << "order: status=" << newOrderResp.data.status << std::endl;

    // Example: univals transfer
    binance::WalletUniversalTransfer transfer;
    transfer.type = binance::UT_MAIN_UMFUTURE;
    transfer.asset = "BNB";
    transfer.amount = 0.9;
    binance::CommonRestResponse<uint64_t> transferResponse;
    binanceRestWallet.universal_transfer(transfer, transferResponse);
    std::cout << "code=" << transferResponse.code << ",msg=" << transferResponse.msg << std::endl;
    std::cout << "tranId" << transferResponse.data << std::endl;

    while(true) {
        std::cout << "Keep Running..." << std::endl;
        info_log("Process Keep Running...");
        std::this_thread::sleep_for(std::chrono::seconds(3));
    }

    return 0;
}
