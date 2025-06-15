#ifndef _ACTUARY_COMMON_CONTAINER_H_
#define _ACTUARY_COMMON_CONTAINER_H_

#include <string>
#include <optional>
#include <cstdint>

using namespace std;

namespace actuary {

    struct ExchangeInfoLite {
        string symbol;
        int pricePrecision;
        int quantityPrecision;
        double minPrice = 0;
        double tickSize = 0;
        double minQty = 0;
        double stepSize = 0;
    };
}

#endif