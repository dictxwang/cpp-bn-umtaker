#include "auto_reset_counter.h"

using namespace std;

bool AutoResetCounter::reset_if_cycle_end(uint64_t now) {

    std::unique_lock<std::shared_mutex> lock(rw_lock);
    if (this->latest_reset_millis + this->reset_duration_millis <= now) {
        // has expired
        this->semaphore_remain_count = this->semaphore_init_count;
        this->latest_reset_millis = now;
        return true;
    }
    return false;
}

bool AutoResetCounter::get_semaphore(int require_count) {
    std::unique_lock<std::shared_mutex> lock(rw_lock);
    if (this->semaphore_remain_count >= require_count) {
        this->semaphore_remain_count -= require_count;
        return true;
    } else {
        return false;
    }
}

void AutoResetCounterBoss::init(int max_capacity, uint64_t refresh_duration_millis=1) {

    this->is_started = false;
    this->max_capacity = max_capacity;
    if (refresh_duration_millis <= 0) {
        this->refresh_duration_millis = 1;
    } else {
        this->refresh_duration_millis = refresh_duration_millis;
    }
}

void AutoResetCounterBoss::refresh_counters() {
    while (this->is_started) {
        std::this_thread::sleep_for(std::chrono::milliseconds(this->refresh_duration_millis));
        uint64_t now = binance::get_current_ms_epoch();
        std::unique_lock<std::shared_mutex> lock(rw_lock);
        for (auto item = this->counter_map.begin(); item != this->counter_map.end(); ++item) {
            item->second->reset_if_cycle_end(now);
        }
    }
}

void AutoResetCounterBoss::start() {
    std::unique_lock<std::shared_mutex> lock(rw_lock);
    if (!this->is_started) {
        this->is_started = true;
        thread refresh_thread(&AutoResetCounterBoss::refresh_counters, this);
        refresh_thread.detach();
        info_log("start semaphore counter boss refresh thread");
    }
}

void AutoResetCounterBoss::stop() {
    std::unique_lock<std::shared_mutex> lock(rw_lock);
    this->is_started = false;
}

optional<shared_ptr<AutoResetCounter>> AutoResetCounterBoss::create_new_counter(int semaphore_count, uint64_t reset_duration_millis) {

    int exists_count = 0;
    std::shared_lock<std::shared_mutex> r_lock(rw_lock);
    exists_count = this->counter_map.size();
    r_lock.unlock();

    if (exists_count >= this->max_capacity) {
        warn_log("cannot create more semaphore counter in this boos");
        return nullopt;
    }

    shared_ptr<AutoResetCounter> counter = make_shared<AutoResetCounter>(semaphore_count, reset_duration_millis);
    std::unique_lock<std::shared_mutex> w_lock(rw_lock);
    this->counter_map[counter->id] = counter;
    w_lock.unlock();
    return counter;
}

optional<shared_ptr<AutoResetCounter>> AutoResetCounterBoss::get_exist_counter(string& counter_id) {

    std::shared_lock<std::shared_mutex> r_lock(rw_lock);
    auto counter = this->counter_map.find(counter_id);
    if (counter == this->counter_map.end()) {
        return nullopt;
    } else {
        return counter->second;
    }
}