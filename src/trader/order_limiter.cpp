#include "order_limiter.h"

using namespace std;

namespace trader {

    void AutoResetOrderLimiter::init(int second_semaphore_count, uint64_t second_reset_duration_millis,
            int minute_semaphore_count, uint64_t minute_reset_duration_millis) {

            
        this->second_semaphore_init_count = second_semaphore_count;
        this->second_semaphore_remain_count = second_semaphore_count;
        this->second_reset_duration_millis = second_reset_duration_millis;
        this->second_latest_reset_millis = binance::get_current_ms_epoch();

        this->minute_semaphore_init_count = minute_semaphore_count;
        this->minute_semaphore_remain_count = minute_semaphore_count;
        this->minute_reset_duration_millis = minute_reset_duration_millis;
        this->minute_latest_reset_millis = binance::get_current_ms_epoch();

        this->id = binance::generate_uuid();
    }

    void AutoResetOrderLimiter::reset_if_cycle_end(uint64_t now) {

        std::unique_lock<std::shared_mutex> lock(rw_lock);
        if (this->second_latest_reset_millis + this->second_reset_duration_millis <= now) {
            // has expired
            this->second_semaphore_remain_count = this->second_semaphore_init_count;
            this->second_latest_reset_millis = now;
        }

        if (this->minute_latest_reset_millis + this->minute_reset_duration_millis <= now) {
            // has expired
            this->minute_semaphore_remain_count = this->minute_semaphore_init_count;
            this->minute_latest_reset_millis = now;
        }
    }

    bool AutoResetOrderLimiter::get_order_semaphore(int require_num) {

        std::unique_lock<std::shared_mutex> lock(rw_lock);
        if (this->second_semaphore_remain_count >= require_num && this->minute_semaphore_remain_count >= require_num) {
            this->second_semaphore_remain_count -= require_num;
            this->minute_semaphore_remain_count -= require_num;
            return true;
        } else {
            return false;
        }
    }

    void AutoResetOrderLimiter::return_order_semaphore(int return_num) {

        std::unique_lock<std::shared_mutex> lock(rw_lock);
        if (this->second_semaphore_remain_count + return_num <= this->second_semaphore_init_count) {
            this->second_semaphore_remain_count += return_num;
        } else {
            // maybe semaphore had been reset, no need receive return semaphore
        }

        if (this->minute_semaphore_remain_count + return_num <= this->minute_semaphore_init_count) {
            this->minute_semaphore_remain_count += return_num;
        } else {
            // do nothing
        }
    }

    void AutoResetOrderLimiterBoss::init(int max_capacity, uint64_t refresh_duration_millis, int second_semaphre_count, int duration_second, int minute_semaphore_count, int duration_minute) {
            
        this->is_started = false;
        this->max_capacity = max_capacity;
        if (refresh_duration_millis <= 0) {
            this->refresh_duration_millis = 1;
        } else {
            this->refresh_duration_millis = refresh_duration_millis;
        }

        // init default limiter
        this->account_limiter = make_unique<AutoResetOrderLimiter>();
        this->account_limiter->init(second_semaphre_count, duration_second*1000, minute_semaphore_count, duration_minute*60*1000);
    }

    void AutoResetOrderLimiterBoss::refresh_all_limiters() {
        
        while (this->is_started) {
            std::this_thread::sleep_for(std::chrono::milliseconds(this->refresh_duration_millis));
            uint64_t now = binance::get_current_ms_epoch();
            std::unique_lock<std::shared_mutex> lock(rw_lock);

            this->account_limiter->reset_if_cycle_end(now);
            for (auto item = this->ip_limiter_map.begin(); item != this->ip_limiter_map.end(); ++item) {
                item->second->reset_if_cycle_end(now);
            }
        }
    }

    void AutoResetOrderLimiterBoss::start() {
        std::unique_lock<std::shared_mutex> lock(rw_lock);
        if (!this->is_started) {
            this->is_started = true;
            thread refresh_thread(&AutoResetOrderLimiterBoss::refresh_all_limiters, this);
            refresh_thread.detach();
            info_log("start oreer limiter boss refresh thread");
        }
    }

    void AutoResetOrderLimiterBoss::stop() {
        std::unique_lock<std::shared_mutex> lock(rw_lock);
        this->is_started = false;
    }
    
    bool AutoResetOrderLimiterBoss::create_ip_limiter(string& local_ip, int second_semaphre_count, int duration_second, int minute_semaphore_count, int duration_minute) {

        int ip_limiter_count = 0;
        bool ip_limiter_exists = false;

        std::shared_lock<std::shared_mutex> r_lock(rw_lock);
        ip_limiter_count = this->ip_limiter_map.size();
        auto exists = this->ip_limiter_map.find(local_ip);
        if (exists != this->ip_limiter_map.end()) {
            ip_limiter_exists = true;
        }
        r_lock.unlock();

        if (ip_limiter_exists) {
            info_log("ip limiter has existed fo {}", local_ip);
            return false;
        }

        if (ip_limiter_count >= this->max_capacity) {
            warn_log("cannot create more order limiter in this boos");
            return false;
        }

        shared_ptr<AutoResetOrderLimiter> ip_limiter = make_shared<AutoResetOrderLimiter>();
        ip_limiter->init(second_semaphre_count, duration_second*1000, minute_semaphore_count, duration_minute*60*1000);
       
        std::unique_lock<std::shared_mutex> w_lock(rw_lock);
        this->ip_limiter_map[local_ip] = ip_limiter;
        w_lock.unlock();

        info_log("create new order limiter for {} with thresholds {} {} {} {}",
            local_ip,
            second_semaphre_count, duration_second*1000,
            minute_semaphore_count, duration_minute*60*1000
        );
        
        return true;
    }

    pair<bool, bool> AutoResetOrderLimiterBoss::get_order_semaphore(string &local_ip, int require_num) {
        
        bool has_account_semaphore, has_ip_semaphore = false;

        std::shared_lock<std::shared_mutex> r_lock(rw_lock);
        has_account_semaphore = this->account_limiter->get_order_semaphore(require_num);
        if (!has_account_semaphore) {
            return pair<bool, bool>(has_account_semaphore, has_ip_semaphore);
        }

        auto ip_limiter = this->ip_limiter_map.find(local_ip);
        if (ip_limiter == this->ip_limiter_map.end()) {
            // not ip limiter
            this->account_limiter->return_order_semaphore(require_num);
            return pair<bool, bool>(has_account_semaphore, has_ip_semaphore);
        }

        has_ip_semaphore = ip_limiter->second->get_order_semaphore(require_num);
        if (!has_ip_semaphore) {
            // no ip semaphore, should return default semaphore
            this->account_limiter->return_order_semaphore(require_num);
            return pair<bool, bool>(has_account_semaphore, has_ip_semaphore);
        }

        return pair<bool, bool>(has_account_semaphore, has_ip_semaphore);
    }
}