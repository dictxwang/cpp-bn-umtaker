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