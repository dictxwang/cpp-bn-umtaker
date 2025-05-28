#include <iostream>
#include <thread>
#include <chrono>
#include "binancecpp/binance.h"
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
    std::cout << messageJson << std::endl;
    return true;
}

void startFuturesOrderService(binance::BinanceFuturesWsClient &futuresWsClient, std::string &apiKey, std::string &secretKey) {
    try {
        futuresWsClient.initOrderService(apiKey, secretKey, false);
        futuresWsClient.setMessageCallback(processFuturesOrderServiceMessage);
        futuresWsClient.startOrderService();
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

int main(int argc, char const *argv[])
{
    std::cout << "00000" << std::endl;
    
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

    std::cout << "<< this is main process >>" << std::endl;

    // Create an instance of BinanceSpotRestClient
    binance::BinanceSpotRestClient binanceSpot;
    binanceSpot.init(config.api_key_hmac, config.secret_key_hmac, false);

    binance::BinanceFuturesRestClient binanceFutures;
    binanceFutures.init(config.api_key_hmac, config.secret_key_hmac, false);

    // binance::CommonRestResponse<uint64_t> timeResp;
    // binanceSpot.setServerTimeOffset(timeResp);
    // std::cout << "Response code: " << timeResp.code << std::endl;
    // std::cout << "Response data " << timeResp.data << std::endl;

    // binance::CommonRestResponse<std::vector<binance::SpotExchangeInfo>> response;
    // std::vector<std::string> instIds;
    // instIds.push_back("BTCUSDT");
    // instIds.push_back("ETHUSDT");
    // binanceSpot.get_exchangeInfo(instIds, response);
    // std::cout << "Response code: " << response.code << std::endl;
    // std::cout << "Response symbols-0.baseAsset: " << response.data.size() << std::endl;

    // binance::CommonRestResponse<binance::SpotAccount> accountResp;
    // binanceSpot.get_account(accountResp);
    // std::cout << "Response code: " << accountResp.code << std::endl;
    // std::cout << "Response msg: " << accountResp.msg << std::endl;
    // std::cout << "Response data: " << accountResp.data.accountType << std::endl;

    binance::BinanceSpotWsClient binanceSpotWs;
    // std::vector<std::string> symbols;
    // symbols.push_back("BTCUSDT");
    // symbols.push_back("ETHUSDT");
    // binanceSpotWs.initBookTicker(false, false);
    // binanceSpotWs.setMessageCallback(processTickerMessage);
    // moodycamel::ConcurrentQueue<std::string> messageChannel;
    // binanceSpotWs.setMessageChannel(&messageChannel);

    // std::thread messagenChannelConsume(startMessageChannelConsume, &messageChannel);
    // std::cout << "After Inited." << std::endl;
    // binanceSpotWs.startBookTicker(symbols);

    // binance::CommonRestResponse<std::string> startUserStreamResp;
    // binanceSpot.start_userDataStream(startUserStreamResp);
    // std::cout << "Response code: " << startUserStreamResp.code << std::endl;
    // std::cout << "Response msg: " << startUserStreamResp.msg << std::endl;
    // std::cout << "Response data: " << startUserStreamResp.data << std::endl;
    
    // std::string listenKey = "2B1i7X0Y7VSyTnLcCBjxpXJTPcMPRcmZxri2L0DkRsnp6aTW4pzFeO9fvDP7";
    // binance::CommonRestResponse<std::string> keepUserStreamResp;
    // binanceSpot.keep_userDataStream(listenKey, keepUserStreamResp);
    // std::cout << "Response code: " << keepUserStreamResp.code << std::endl;
    // std::cout << "Response msg: " << keepUserStreamResp.msg << std::endl;

    // binanceSpotWs.initUserDataStreamV1(config.api_key_hmac, config.secret_key_hmac, false);
    // binanceSpotWs.startUserDataStreamV1(processUserDataMessage, listenKey);
    // std::cout << "end user data stream" << std::endl;

    // binanceSpotWs.initUserDataStream(config.api_key_ed25519, config.secret_key_ed25519, false);
    // binanceSpotWs.setMessageCallback(processUserDataMessage);
    // std::pair<bool, string> startResult = binanceSpotWs.startUserDataStream();
    // std::cout << "start result: " << startResult.first << ", msg=" << startResult.second << std::endl;

    // binance::CommonRestResponse<uint64_t> futuresTimeResp;
    // binanceFutures.setServerTimeOffset(futuresTimeResp);
    // std::cout << "Response code futures set time-offset: " << futuresTimeResp.code << std::endl;
    // std::cout << "Response data futures set time-offset:" << futuresTimeResp.data << std::endl;

    // binance::CommonRestResponse<std::vector<binance::FuturesExchangeInfo>> futuresResponse;
    // binanceFutures.get_exchangeInfo(futuresResponse);
    // std::cout << "Response code: " << futuresResponse.code << std::endl;
    // std::cout << "Response symbols-0.symbol: " << futuresResponse.data[0].symbol << std::endl;

    // binance::CommonRestResponse<binance::FuturesAccount> fururesAcccountResponse;
    // binanceFutures.get_account_v2(fururesAcccountResponse);
    // std::cout << "Response code: " << fururesAcccountResponse.code << std::endl;
    // std::cout << "Response account canWithdraw: " << fururesAcccountResponse.data.canWithdraw << std::endl;

    // binance::CommonRestResponse<bool> futuresMultiAssetResponse;
    // binanceFutures.get_multiAssetMargin(futuresMultiAssetResponse);
    // std::cout << "Response-get_multiAssetMargin code: " << futuresMultiAssetResponse.code << std::endl;
    // std::cout << "Response-get_multiAssetMargin msg: " << futuresMultiAssetResponse.msg << std::endl;
    // std::cout << "Response-get_multiAssetMargin data: " << futuresMultiAssetResponse.data << std::endl;

    // binance::CommonRestResponse<bool> futuresFeeBurnResponse;
    // binanceFutures.get_bnbFeeBurn(futuresFeeBurnResponse);
    // std::cout << "Response-get_bnbFeeBurn code: " << futuresMultiAssetResponse.code << std::endl;
    // std::cout << "Response-get_bnbFeeBurn msg: " << futuresMultiAssetResponse.msg << std::endl;
    // std::cout << "Response-get_bnbFeeBurn data: " << futuresMultiAssetResponse.data << std::endl;

    // binance::CommonRestResponse<bool> futuresFeeBurnToggleResponse;
    // binanceFutures.toggle_bnbFeeBurn(true, futuresFeeBurnToggleResponse);
    // std::cout << "Response-toggle_bnbFeeBurn code: " << futuresFeeBurnToggleResponse.code << std::endl;
    // std::cout << "Response-toggle_bnbFeeBurn msg: " << futuresFeeBurnToggleResponse.msg << std::endl;
    // std::cout << "Response-toggle_bnbFeeBurn data: " << futuresFeeBurnToggleResponse.data << std::endl;

    binance::BinanceFuturesWsClient futuresWsClient;
    // futuresWsClient.initBookTickerV1(false, false);
    // std::vector<string> futuresSymbols;
    // futuresSymbols.push_back("BTCUSDT");
    // futuresSymbols.push_back("ETHUSDT");
    // // futuresWsClient.startBookTickerV1(processFuturesTickerMessage, futuresSymbols);

    // futuresWsClient.initMarkPriceV1(false, false);
    // futuresWsClient.startAllMarkPricesV1(processMarkPriceMessage, binance::WsMarkPriceInterval::WsMPThreeSeconds);
    // futuresWsClient.startMarkPriceV1(processMarkPriceMessage, futuresSymbols, binance::WsMarkPriceInterval::WsMPThreeSeconds);

    // binance::CommonRestResponse<std::string> startFuturesUserStreamResp;
    // binanceFutures.start_userDataStream(startFuturesUserStreamResp);
    // std::cout << "Response code: " << startFuturesUserStreamResp.code << std::endl;
    // std::cout << "Response msg: " << startFuturesUserStreamResp.msg << std::endl;
    // std::cout << "Response data: " << startFuturesUserStreamResp.data << std::endl;
    
    std::string futuresListenKey = "SkF9KFpQMstAbYf4KXtHD60J3nq1TYrIVYWx3oujAf2Sk2oPssRrwVZosUKGv3a3";
    // binance::CommonRestResponse<std::string> keepFuturesUserStreamResp;
    // binanceFutures.keep_userDataStream(futuresListenKey, keepFuturesUserStreamResp);
    // std::cout << "Response code: " << keepFuturesUserStreamResp.code << std::endl;
    // std::cout << "Response msg: " << keepFuturesUserStreamResp.msg << std::endl;

    // futuresWsClient.initUserDataStreamV1(config.api_key_hmac, config.secret_key_hmac, false);
    // futuresWsClient.startUserDataStreamV1(processFuturesUserDataMessage, futuresListenKey);

    // futuresWsClient.initUserDataStream(config.api_key_ed25519, config.secret_key_ed25519, false);
    // futuresWsClient.startUserDataStream(processFuturesUserDataMessage);

    // futuresWsClient.initOrderService(config.api_key_ed25519, config.secret_key_ed25519, false);

    // std::thread futureOrderThread(startFuturesOrderService, std::ref(futuresWsClient), std::ref(config.api_key_ed25519), std::ref(config.secret_key_ed25519));
    // futureOrderThread.join();
    // std::cout << "start order service" << std::endl;

    // std::cout << "local time: " << binance::get_current_ms_epoch() << std::endl;
    // std::this_thread::sleep_for(std::chrono::seconds(10));

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
    //     bool placeResult = futuresWsClient.placeOrder(order);
    //     std::cout << "place result: " << placeResult << std::endl;
    // } catch (std::exception &e) {
    //     std::cout << "fail to place order: " << e.what() << std::endl;
    // }
    
    // std::this_thread::sleep_for(std::chrono::seconds(5));
    
    // binance::FuturesCancelOrder cancelOrder;
    // cancelOrder.origClientOrderId = "a1234567890";
    // cancelOrder.symbol = "BTCUSDT";

    // try {
    //     bool cancelResult = futuresWsClient.cancelOrder(cancelOrder);
    //     std::cout << "cancel result: " << cancelResult << std::endl;
    // } catch (std::exception &e) {
    //     std::cout << "fail to cancel order: " << e.what() << std::endl;
    // }
    
    std::thread zmqSender(startZMQSender, std::ref(config.zmq_ipc));
    std::this_thread::sleep_for(std::chrono::seconds(3));
    std::thread zmqReceiver(startZMQReceiver, std::ref(config.zmq_ipc));

    while(true) {
        std::cout << "Keep Running..." << std::endl;
        info_log("Process Keep Running...");
        std::this_thread::sleep_for(std::chrono::seconds(3));
    }

    return 0;
}
