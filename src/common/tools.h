#ifndef _COMMON_TOOLS_H_
#define _COMMON_TOOLS_H_

#include <string>
#include <iostream>
#include <sys/time.h>
#include <cstdint>
#include <cmath>
#include <chrono>
#include <sstream>
#include <ctime>
#include <iomanip>

bool str_ends_with(const std::string& str, const std::string& suffix);

std::string gen_client_order_id(bool is_buy_side, bool adjusted_price, int ticker_delay_millis, bool is_close_position);

double decimal_process(double value, int precision);

int calculate_precision_by_min_step(double min_step);

std::string get_utc_time_tring();

#endif  