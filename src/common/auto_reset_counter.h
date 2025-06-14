#ifndef _COMMON_AUTORESET_COUNTER_H_
#define _COMMON_AUTORESET_COUNTER_H_

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

class AutoResetCounter {
public:
    AutoResetCounter(int semaphore_count, uint64_t reset_duration_millis) {
        this->semaphore_init_count = semaphore_count;
        this->semaphore_remain_count = semaphore_count;
        this->reset_duration_millis = reset_duration_millis;
        this->latest_reset_millis = binance::get_current_ms_epoch();
        this->id = binance::generate_uuid();
    }
    ~AutoResetCounter() {}

private:
    string id;
    uint64_t reset_duration_millis;
    uint64_t latest_reset_millis;
    int semaphore_init_count;
    int semaphore_remain_count;
    shared_mutex rw_lock;

private:
    bool reset_if_cycle_end(uint64_t now);

    friend class AutoResetCounterBoss;

public:
    bool get_semaphore(int require_count);
};

class AutoResetCounterBoss {
public:
    AutoResetCounterBoss() {}
    ~AutoResetCounterBoss() {
        this->is_started = false;
        this->counter_map.clear();
    }

private:
    int max_capacity;
    uint64_t refresh_duration_millis;
    bool is_started;
    unordered_map<string, shared_ptr<AutoResetCounter>> counter_map;
    shared_mutex rw_lock;

private:
    void refresh_counters();

public:
    void init(int max_capacity, uint64_t refresh_duration_millis);
    void start();
    void stop();
    optional<shared_ptr<AutoResetCounter>> create_new_counter(int semaphore_count, uint64_t reset_duration_millis);
    optional<shared_ptr<AutoResetCounter>> get_exist_counter(string& counter_id);
};

#endif