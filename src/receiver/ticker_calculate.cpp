#include "ticker_calculate.h"

namespace receiver {

    void start_ticker_calculate(ReceiverConfig& config, GlobalContext &context) {

        std::thread process_ticker_price_offset(process_ticker_info_price_offset, std::ref(config), std::ref(context));
        process_ticker_price_offset.detach();

        std::thread process_early_run_calculate(process_early_run_threshold_calculate, std::ref(config), std::ref(context));
        process_early_run_calculate.detach();

        std::thread process_beta_calculate(process_beta_threshold_calculate, std::ref(config), std::ref(context));
        process_beta_calculate.detach();
    }

    void process_early_run_threshold_calculate(ReceiverConfig& config, GlobalContext &context) {
        RandomIntGen rand;
        rand.init(0, 10000);

        while (true) {
            std::string inst_id;
            while (!context.get_early_run_calculation_channel()->try_dequeue(inst_id)) {
                // Retry if the queue is empty
            }

            std::string base_asset;
            if (str_ends_with(inst_id, config.benchmark_quote_asset)) {
                base_asset = inst_id.substr(0, inst_id.size() - config.benchmark_quote_asset.size());
            } else {
                base_asset = inst_id.substr(0, inst_id.size() - config.follower_quote_asset.size());
            }

            std::vector<PriceOffset> offset_list = context.get_price_offset_composite().get_price_offset_list(base_asset);
            if (offset_list.size() < config.calculate_sma_interval_seconds) {
                continue;
            }

            std::vector<double> avg_price_diff_list;
            std::vector<double> bid_ask_price_diff_list;
            std::vector<double> ask_bid_price_diff_list;

            for (size_t i = 0; i < offset_list.size(); ++i) {
                avg_price_diff_list.push_back(offset_list[i].avg_price_diff);
                bid_ask_price_diff_list.push_back(offset_list[i].bid_ask_price_diff);
                ask_bid_price_diff_list.push_back(offset_list[i].ask_bid_price_diff);
            }

            std::sort(avg_price_diff_list.begin(), avg_price_diff_list.end());
            std::sort(bid_ask_price_diff_list.begin(), bid_ask_price_diff_list.end());
            std::sort(ask_bid_price_diff_list.begin(), ask_bid_price_diff_list.end());

            size_t median_index = offset_list.size() / 2;

            EarlyRunThreshold threshold;
            threshold.avg_price_diff_median = avg_price_diff_list[median_index];
            threshold.bid_ask_price_diff_median = bid_ask_price_diff_list[median_index];
            threshold.ask_bid_price_diff_median = ask_bid_price_diff_list[median_index];
            threshold.price_offset_length = offset_list.size();
            threshold.current_time_mills = binance::get_current_ms_epoch();

            context.get_early_run_threshold_composite().update(base_asset, threshold);

            if (rand.randInt() < 100) {
                info_log("process early-run threshold: inst_id={} base={} avg_price_diff_median={} bid_ask_price_diff_median={} ask_bid_price_diff_median={} price_offset_length={} current_time_mills={}",
                    inst_id, base_asset, threshold.avg_price_diff_median, threshold.bid_ask_price_diff_median,
                    threshold.ask_bid_price_diff_median, threshold.price_offset_length, threshold.current_time_mills);
            }
        }
    }

    void process_beta_threshold_calculate(ReceiverConfig& config, GlobalContext &context) {
        RandomIntGen rand;
        rand.init(0, 10000);

        while (true) {
            std::string inst_id;
            while (!context.get_beta_calculation_channel()->try_dequeue(inst_id)) {
                // Retry if the queue is empty
            }

            std::string base_asset;
            if (str_ends_with(inst_id, config.benchmark_quote_asset)) {
                base_asset = inst_id.substr(0, inst_id.size() - config.benchmark_quote_asset.size());
            } else {
                base_asset = inst_id.substr(0, inst_id.size() - config.follower_quote_asset.size());
            }

            auto inst_config = context.get_inst_config().inst_map.find(base_asset);
            if (inst_config == context.get_inst_config().inst_map.end()) {
                warn_log("not found inst config for {}/{}", base_asset, inst_id);
                continue;
            }

            double volatility_a = (*inst_config).second.volatility_a;
            double volatility_b = (*inst_config).second.volatility_b;
            double volatility_c = (*inst_config).second.volatility_c;
            double beta = (*inst_config).second.beta;

            double max_bid = std::numeric_limits<double>::min();
            double min_bid = std::numeric_limits<double>::max();

            double max_ask = std::numeric_limits<double>::min();
            double min_ask = std::numeric_limits<double>::max();

            double max_avg = std::numeric_limits<double>::min();
            double min_avg = std::numeric_limits<double>::max();

            // TODO copy all ticker
            // context.get_follower_ticker_composite().
        }
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

            // Send ticker info to early run parameter calculator
            bool result = (*context.get_early_run_calculation_channel()).try_enqueue(ticker_info.inst_id);
            if (!result) {
                warn_log("can not enqueue early run calcalation: {}", ticker_info.inst_id);
            }

            if (rand.randInt() < 100) {
                info_log("process ticker for price offset: symbol={} bid={} bid_size={} ask={} ask_size={} up_ts={}",
                    ticker_info.inst_id, ticker_info.bid_price, ticker_info.bid_volume, ticker_info.ask_price, ticker_info.ask_volume, ticker_info.update_time_millis);
            }
        }
    } 

}