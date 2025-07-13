#ifndef _EVALUATOR_TICKER_CONTAINER_H_
#define _EVALUATOR_TICKER_CONTAINER_H_

#include <string>
#include <cstdint>
#include <unordered_map>
#include <mutex>
#include <shared_mutex>
#include <optional>

using namespace std;

namespace evaluator {

    struct TickerLite {
        string symbol;
        double ask;
        double bid;
        uint64_t update_id;
        uint64_t update_time_millis;

        void copy_self(TickerLite& lite);
    };

    class TickerLiteComposite {
    public:
        TickerLiteComposite() {}
        ~TickerLiteComposite() {}
    
    private:
        unordered_map<string, TickerLite> ticker_map;
        shared_mutex rw_lock;
    
    public:
        void update_ticker(TickerLite lite);
        optional<TickerLite> get_ticker(string symbol);
    };
}

#endif