#include "ticker_calculate.h"

namespace receiver {

    void start_ticker_calculate(ReceiverConfig& config, GlobalContext &context) {

        std::thread process_ticker_price_offset(process_ticker_info_price_offset, std::ref(config), std::ref(context));
        process_ticker_price_offset.detach();
    }

    void process_ticker_info_price_offset(ReceiverConfig& config, GlobalContext &context) {
        
        RandomIntGen rand;
        rand.init(0, 10000);

        while (true) {
            UmTickerInfo ticker_info;
            while (!context.get_ticker_info_channel()->try_dequeue(ticker_info)) {
                // Retry if the queue is empty
            }

            if (str_ends_with(ticker_info.inst_id, config.benchmark_quote_asset)) {
                std::string base_asset = ticker_info.inst_id.substr(0, ticker_info.inst_id.size() - config.benchmark_quote_asset.size());
                std::string follower_inst_id = base_asset + config.follower_quote_asset;
                optional<UmTickerInfo> follower_ticker_info = context.get_follower_ticker_composite().get_lastest_ticker(follower_inst_id);
                if (follower_ticker_info.has_value()) {
                    context.get_price_offset_composite().update_price_offset(base_asset, ticker_info, follower_ticker_info.value());
                }
            } else if (str_ends_with(ticker_info.inst_id, config.follower_quote_asset)) {
                std::string base_asset = ticker_info.inst_id.substr(0, ticker_info.inst_id.size() - config.follower_quote_asset.size());
                std::string benchmark_inst_id = base_asset + config.benchmark_quote_asset;
                optional<UmTickerInfo> benchmark_ticker_info = context.get_benchmark_ticker_composite().get_lastest_ticker(benchmark_inst_id);
                if (benchmark_ticker_info.has_value()) {
                    context.get_price_offset_composite().update_price_offset(base_asset, benchmark_ticker_info.value(), ticker_info);
                }
            }

            if (rand.randInt() < 100) {
                info_log("process ticker for price offset: symbol={} bid={} bid_size={} ask={} ask_size={} up_ts={}",
                    ticker_info.inst_id, ticker_info.bid_price, ticker_info.bid_volume, ticker_info.ask_price, ticker_info.ask_volume, ticker_info.update_time_millis);
            }
        }
    } 

}