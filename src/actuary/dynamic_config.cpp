#include "dynamic_config.h"

using namespace std;

namespace actuary {

    DynamicConfig::DynamicConfig(bool stop_make_order) {
        if (stop_make_order) {
            this->stop_reasons.insert(STOP_REASON_STATIC_CONFIG);
            this->stop_order = true;
            this->stop_open_order = true;
        } else {
            this->stop_order = false;
            this->stop_open_order = false;
        }
    }
    
    void DynamicConfig::stop_make_order(const string& stopReason) {

        std::unique_lock<std::shared_mutex> w_lock(this->rw_lock);
        if (this->stop_reasons.find(stopReason) == this->stop_reasons.end()) {
            this->stop_reasons.insert(stopReason);
        }
        this->stop_order = true;
        w_lock.unlock();
    }

    bool DynamicConfig::resume_make_order(const string& stopReason) {

        bool resumed = false;
        std::unique_lock<std::shared_mutex> w_lock(this->rw_lock);
        if (this->stop_reasons.find(stopReason) != this->stop_reasons.end()) {
            this->stop_reasons.erase(stopReason);
        }
        if (this->stop_reasons.size() == 0) {
            this->stop_order = false;
        }
        resumed = !this->stop_order;
        w_lock.unlock();
        return resumed;
    }

    bool DynamicConfig::is_stop_make_order() {
        return this->stop_order;
    }
    bool DynamicConfig::is_stop_make_order_as_reason(string stopReason) {

        if (!this->stop_order) {
            return false;
        }

        bool stopped = false;
        std::shared_lock<std::shared_mutex> r_lock(this->rw_lock);
        if (this->stop_reasons.size() > 0 && this->stop_reasons.find(stopReason) != this->stop_reasons.end()) {
            stopped = true;
        }
        r_lock.unlock();

        return stopped;
    }

    void DynamicConfig::stop_make_open_order() {

        std::unique_lock<std::shared_mutex> w_lock(this->rw_lock);
        this->stop_open_order = true;
        w_lock.unlock();
    }

    void DynamicConfig::resume_make_open_order() {

        std::unique_lock<std::shared_mutex> w_lock(this->rw_lock);
        this->stop_open_order = false;
        w_lock.unlock();
    }

    bool DynamicConfig::is_stop_make_open_order() {
        return this->stop_open_order;
    }
}