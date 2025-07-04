#include "tools.h"

bool str_ends_with(const std::string& str, const std::string& suffix) {
    // Check if the suffix is longer than the string
    if (suffix.size() > str.size()) {
        return false;
    }

    // Use rfind to check if the suffix matches the end of the string
    return str.rfind(suffix) == (str.size() - suffix.size());
}

std::string gen_client_order_id(bool is_buy_side, bool adjusted_price, int ticker_delay_millis, bool is_close_position) {
    struct timeval tv;
    gettimeofday(&tv, NULL); 
    uint64_t now = tv.tv_sec * 1000 + tv.tv_usec / 1000 ;
    char adjusted = adjusted_price ? 'y' : 'n';
    char close = is_close_position ? 'y' : 'n';
    std::string side = is_buy_side ? "b" : "s";

    std::string client_id = side + "_" + close + "_" + std::to_string(now) + "_" + adjusted + "_" + std::to_string(ticker_delay_millis);

    return client_id;
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