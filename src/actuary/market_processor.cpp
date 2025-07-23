#include "market_processor.h"

using namespace std;

namespace actuary {
    
    vector<ExchangeInfoLite> load_follower_exchangeInfo(ActuaryConfig &config, binance::BinanceFuturesRestClient &restClient) {
        binance::CommonRestResponse<std::vector<binance::FuturesExchangeInfo>> response;
        restClient.get_exchangeInfo(response);

        if (response.code != binance::RestCodeOK) {
            err_log("fail to load exchangeInfo: {} {}", response.code, response.msg);
            exit(-1);
        }

        vector<ExchangeInfoLite> lites;
        for (binance::FuturesExchangeInfo info : response.data) {
            // std::cout << info.symbol << ",pricePrecision=" << info.pricePrecision << ",quantityPrecision=" << info.quantityPrecision << ",tickSize=" << info.tickSize << ",stepSize=" << info.stepSize << std::endl;
            if (info.quoteAsset == config.follower_quote_asset) {
                int price_precision = calculate_precision_by_min_step(info.tickSize);
                int quantity_precision = calculate_precision_by_min_step(info.stepSize);
                ExchangeInfoLite lite;
                lite.symbol = info.symbol;
                lite.baseAsset = info.baseAsset;
                lite.pricePrecision = std::min(info.pricePrecision, price_precision);
                lite.quantityPrecision = std::min(info.quantityPrecision, quantity_precision);
                lite.minPrice = info.minPrice;
                lite.tickSize = info.tickSize;
                lite.minQty = info.minQty;
                lite.stepSize = info.stepSize;

                lites.push_back(lite);
            }
        }
        return lites;
    }

    unordered_map<string, ExchangeInfoLite> load_benchmark_exchangeInfo(ActuaryConfig &config, binance::BinanceFuturesRestClient &restClient) {
        binance::CommonRestResponse<std::vector<binance::FuturesExchangeInfo>> response;
        restClient.get_exchangeInfo(response);

        if (response.code != binance::RestCodeOK) {
            err_log("fail to load exchangeInfo: {} {}", response.code, response.msg);
            exit(-1);
        }

        unordered_map<string, ExchangeInfoLite> lite_map;
        for (binance::FuturesExchangeInfo info : response.data) {
            // std::cout << info.symbol << ",pricePrecision=" << info.pricePrecision << ",quantityPrecision=" << info.quantityPrecision << ",tickSize=" << info.tickSize << ",stepSize=" << info.stepSize << std::endl;
            if (info.quoteAsset == config.benchmark_quote_asset) {
                int price_precision = calculate_precision_by_min_step(info.tickSize);
                int quantity_precision = calculate_precision_by_min_step(info.stepSize);
                ExchangeInfoLite lite;
                lite.symbol = info.symbol;
                lite.baseAsset = info.baseAsset;
                lite.pricePrecision = std::min(info.pricePrecision, price_precision);
                lite.quantityPrecision = std::min(info.quantityPrecision, quantity_precision);
                lite.minPrice = info.minPrice;
                lite.tickSize = info.tickSize;
                lite.minQty = info.minQty;
                lite.stepSize = info.stepSize;

                lite_map.insert({info.baseAsset, lite});
            }
        }
        return lite_map;
    }
}