#ifndef _ACTUARY_COMMON_CONTAINER_H_
#define _ACTUARY_COMMON_CONTAINER_H_

#include <string>
#include <optional>
#include <cstdint>

using namespace std;

namespace actuary {

    struct ExchangeInfoLite {
        string symbol;
        string baseAsset;
        int pricePrecision;
        int quantityPrecision;
        double minPrice = 0;
        double tickSize = 0;
        double minQty = 0;
        double stepSize = 0;
    };

    struct StatOrderLite {
        string accountFlag;
        string symbol;
        string orderSide;
        string clientOrderId;
        string averagePrice;
        string filledSize;
        string commissionRate;
        uint64_t systemTimestamp = 0;
    };
}

#endif