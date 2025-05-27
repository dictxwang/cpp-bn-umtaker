#ifndef _COMMON_TOOLS_H_
#define _COMMON_TOOLS_H_

#include <string>
#include <iostream>
#include <sys/time.h>
#include <cstdint>

bool str_ends_with(const std::string& str, const std::string& suffix);

std::string gen_client_order_id(bool is_buy_size);

#endif  