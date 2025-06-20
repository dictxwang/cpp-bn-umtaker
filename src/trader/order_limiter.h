#ifndef _TRADER_ORDER_LIMITER_H_
#define _TRADER_ORDER_LIMITER_H_

#include <thread>
#include <chrono>
#include <unordered_map>
#include <mutex>
#include <shared_mutex>
#include <cstdint>
#include <memory>
#include "binancecpp/util/binance_tool.h"
#include "logger/logger.h"

using namespace std;

namespace trader {

    class AutoResetOrderLimiter {
    public:
        AutoResetOrderLimiter() {
        }
        ~AutoResetOrderLimiter() {}

    private:
        string id;
        int second_semaphore_init_count;
        int second_semaphore_remain_count;
        uint64_t second_reset_duration_millis;
        uint64_t second_latest_reset_millis;

        int minute_semaphore_init_count;
        int minute_semaphore_remain_count;
        uint64_t minute_reset_duration_millis;
        uint64_t minute_latest_reset_millis;

        shared_mutex rw_lock;

    private:
        void init(int second_semaphore_count, uint64_t second_reset_duration_millis,
            int minute_semaphore_count, uint64_t minute_reset_duration_millis);
        void reset_if_cycle_end(uint64_t now);
        bool get_order_semaphore(int require_num);
        void return_order_semaphore(int require_num);

        friend class AutoResetOrderLimiterBoss;
    };

    class AutoResetOrderLimiterBoss{
    public:
        AutoResetOrderLimiterBoss() {}
        ~AutoResetOrderLimiterBoss() {
            this->is_started = false;
            this->ip_limiter_map.clear();
        }

    private:
        int max_capacity;
        uint64_t refresh_duration_millis;
        bool is_started;
        unique_ptr<AutoResetOrderLimiter> default_limiter;
        unordered_map<string, shared_ptr<AutoResetOrderLimiter>> ip_limiter_map;
        shared_mutex rw_lock;

    private:
        void refresh_all_limiters();

    public:
        void init(int max_capacity, uint64_t refresh_duration_millis, int second_semaphre_count, int duration_second, int minute_semaphore_count, int duration_minute);
        void start();
        void stop();
        bool create_ip_limiter(string &local_ip, int second_semaphre_count, int duration_second, int minute_semaphore_count, int duration_minute);
        bool get_order_semaphore(string &local_ip, int require_num);
    };
}

#endif