#include "tools.h"

bool str_ends_with(const std::string& str, const std::string& suffix) {
    // Check if the suffix is longer than the string
    if (suffix.size() > str.size()) {
        return false;
    }

    // Use rfind to check if the suffix matches the end of the string
    return str.rfind(suffix) == (str.size() - suffix.size());
}

std::string gen_client_order_id(bool is_buy_side) {
    struct timeval tv;
    gettimeofday(&tv, NULL); 
    uint64_t now = tv.tv_sec * 1000 + tv.tv_usec / 1000 ;
    if (is_buy_side) {
        return 'b' + std::to_string(now);
    } else {
        return 's' + std::to_string(now);
    }
}

double decimal_process(double value, int precision) {
    if (precision == 0) {
        return int(value);
    } else {
        int64_t pow = std::pow(10, precision);
        return double(int64_t(value * pow)) / double(pow);
    }
}

int calculate_precision_by_min_step(double min_step) {
    if (min_step >= 1) {
        return 0;
    } else {
        int precision = 0;
        do {
            precision = precision + 1;
            min_step = min_step * 10;
        } while (min_step < 1);
        return precision;
    }
}