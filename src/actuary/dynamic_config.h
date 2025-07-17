#ifndef _ACTUARY_DYANAMIC_H_
#define _ACTUARY_DYANAMIC_H_

#include <set>
#include <iostream>
#include <mutex>
#include <shared_mutex>

using namespace std;

namespace actuary {

    const string STOP_REASON_STATIC_CONFIG = "STOP_REASON_STATIC_CONFIG";
    const string STOP_REASON_ACCOUNT_META = "STOP_REASON_ACCOUNT_META";
    // const string STOP_REASON_MARGIN_LIMITED = "STOP_REASON_MARGIN_LIMITED";
    const string STOP_REASON_BNB = "STOP_REASON_BNB";
    const string STOP_REASON_BNB_SHORTAGE = "STOP_REASON_BNB_SHORTAGE";

    class DynamicConfig {
    public:
        DynamicConfig(bool stop_make_order);
        ~DynamicConfig() {}
    
    private:
        bool stop_order;
        bool stop_open_order;
        set<string> stop_reasons;
        shared_mutex rw_lock;
    
    public:
        void stop_make_order(const string& stopReason);
        bool resume_make_order(const string& stopReason);
        bool is_stop_make_order();
        bool is_stop_make_order_as_reason(string stopReason);

        void stop_make_open_order();
        void resume_make_open_order();
        bool is_stop_make_open_order();
    };
}
#endif