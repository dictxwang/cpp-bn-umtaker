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

            EarlyRunThreshold threshold;
            if (config.early_run_calculation_type == Early_Run_Calculation_Type_Median) {

                // calculate early run threshold by price offset median
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

                threshold.avg_price_diff_median = avg_price_diff_list[median_index];
                threshold.bid_ask_price_diff_median = bid_ask_price_diff_list[median_index];
                threshold.ask_bid_price_diff_median = ask_bid_price_diff_list[median_index];
                threshold.price_offset_length = offset_list.size();
                threshold.current_time_mills = binance::get_current_ms_epoch();
            } else {

                // calculate early run threshold by average
                auto avg_result = context.get_price_offset_composite().get_price_avg_result(base_asset);
                if (!avg_result.has_value()) {
                    continue;
                }
                if (avg_result->price_offset_length < config.calculate_sma_interval_seconds) {
                    continue;
                }
                threshold.avg_price_diff_median = avg_result->avg_avg_price_diff;
                threshold.bid_ask_price_diff_median = avg_result->avg_bid_ask_price_diff;
                threshold.ask_bid_price_diff_median = avg_result->avg_ask_bid_price_diff;
                threshold.price_offset_length = avg_result->price_offset_length;
                threshold.current_time_mills = binance::get_current_ms_epoch();
            }

            context.get_early_run_threshold_composite().update(base_asset, threshold);

            // sotre into share memory
            auto address = context.get_shm_threshold_mapping().find(base_asset);
            int update_shm = 0;
            if (address != context.get_shm_threshold_mapping().end()) {
                shm_mng::EarlyRunThresholdShm threshold_shm;
                threshold_shm.avg_median = threshold.avg_price_diff_median;
                threshold_shm.bid_ask_median = threshold.bid_ask_price_diff_median;
                threshold_shm.ask_bid_median = threshold.ask_bid_price_diff_median;
                threshold_shm.time_mills = threshold.current_time_mills;
                update_shm = shm_mng::early_run_shm_writer_update(context.get_shm_store_info().early_run_start, (*address).second, threshold_shm);
            }

            if (rand.randInt() < 20) {
                info_log("process early-run threshold: type={} inst_id={} base={} avg_price_diff_median={} bid_ask_price_diff_median={} ask_bid_price_diff_median={} price_offset_length={} current_time_mills={} update_shm={}",
                    config.early_run_calculation_type, inst_id, base_asset, threshold.avg_price_diff_median, threshold.bid_ask_price_diff_median,
                    threshold.ask_bid_price_diff_median, threshold.price_offset_length, threshold.current_time_mills, update_shm);
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

            int rand_value = rand.randInt();
            uint64_t now = binance::get_current_ms_epoch();
            TickerRole role;
            std::string base_asset;
            InstConfigItem inst_config;
            TickerMinMaxResult min_max_result;
            bool runtime = false;

            if (str_ends_with(inst_id, config.benchmark_quote_asset)) {
                base_asset = inst_id.substr(0, inst_id.size() - config.benchmark_quote_asset.size());
                role = TickerRole::Benchmark;
                auto inst_config_auto = context.get_benchmark_inst_config().inst_map.find(base_asset);
                if (inst_config_auto == context.get_benchmark_inst_config().inst_map.end()) {
                    warn_log("not found benchmark inst config for {}/{}", base_asset, inst_id);
                    continue;
                }
                inst_config = inst_config_auto->second;
                
                auto min_max_result_auto = context.get_benchmark_ticker_composite().get_min_max_result(inst_id);
                if (min_max_result_auto.has_value()) {
                    min_max_result = min_max_result_auto.value();
                }
            } else {
                base_asset = inst_id.substr(0, inst_id.size() - config.follower_quote_asset.size());
                role = TickerRole::Follower;
                auto inst_config_auto = context.get_follower_inst_config().inst_map.find(base_asset);
                if (inst_config_auto == context.get_follower_inst_config().inst_map.end()) {
                    warn_log("not found follower inst config for {}/{}", base_asset, inst_id);
                    continue;
                }
                inst_config = inst_config_auto->second;

                auto min_max_result_auto = context.get_follower_ticker_composite().get_min_max_result(inst_id);
                if (min_max_result_auto.has_value()) {
                    min_max_result = min_max_result_auto.value();
                }
            }

            if (!min_max_result.validity_min_max 
                || min_max_result.ticker_length < config.calculate_sma_interval_seconds) {
                
                runtime = true;
                // require try to calculate min max
                std::vector<UmTickerInfo> ticker_list;
                if (role == TickerRole::Benchmark) {
                    ticker_list = context.get_benchmark_ticker_composite().copy_ticker_list_after(inst_id, (now - config.calculate_sma_interval_seconds*1000));
                } else {
                    ticker_list = context.get_follower_ticker_composite().copy_ticker_list_after(inst_id, (now - config.calculate_sma_interval_seconds*1000));
                }

                if (ticker_list.size() < config.calculate_sma_interval_seconds) {
                    if (rand_value < 10) {
                        warn_log("no enough tickers for beta calculation: {}", ticker_list.size());
                    }
                    continue;
                }

                // double volatility_a = (*inst_config).second.volatility_a;
                // double volatility_b = (*inst_config).second.volatility_b;
                // double volatility_c = (*inst_config).second.volatility_c;
                // double beta = (*inst_config).second.beta;

                double max_bid = std::numeric_limits<double>::min();
                double min_bid = std::numeric_limits<double>::max();

                double max_ask = std::numeric_limits<double>::min();
                double min_ask = std::numeric_limits<double>::max();

                double max_avg = std::numeric_limits<double>::min();
                double min_avg = std::numeric_limits<double>::max();


                for (size_t i = 0; i < ticker_list.size(); i++) {
                    if (ticker_list[i].bid_price > max_bid) {
                        max_bid = ticker_list[i].bid_price;
                    }
                    if (ticker_list[i].bid_price < min_bid) {
                        min_bid = ticker_list[i].bid_price;
                    }
                    if (ticker_list[i].ask_price > max_ask) {
                        max_ask = ticker_list[i].ask_price;
                    }
                    if (ticker_list[i].ask_price < min_ask) {
                        min_ask = ticker_list[i].ask_price;
                    }
                    if (ticker_list[i].avg_price > max_avg) {
                        max_avg = ticker_list[i].avg_price;
                    }
                    if (ticker_list[i].avg_price < min_avg) {
                        min_avg = ticker_list[i].avg_price;
                    }
                }

                min_max_result.min_bid_price = min_bid;
                min_max_result.max_bid_price = max_bid;
                min_max_result.min_ask_price = min_ask;
                min_max_result.max_ask_price = max_ask;
                min_max_result.min_avg_price = min_avg;
                min_max_result.max_avg_price = max_avg;
                min_max_result.validity_min_max = true;
            }

            if (min_max_result.min_bid_price <= 0 || min_max_result.min_ask_price <= 0 || min_max_result.min_avg_price) {
                warn_log("invalid ticker min max price: {} {} {}", min_max_result.min_bid_price, min_max_result.min_ask_price, min_max_result.min_avg_price);
                continue;
            }

            double bid_volatility = (min_max_result.max_bid_price - min_max_result.min_bid_price) / min_max_result.min_bid_price;
            double bid_volatility_multiplier = calculate_volatility_multiplier(bid_volatility, inst_config);
            double ask_volatility = (min_max_result.max_ask_price - min_max_result.min_ask_price) / min_max_result.min_ask_price;
            double ask_volatility_multiplier = calculate_volatility_multiplier(ask_volatility, inst_config);
            double avg_volatility = (min_max_result.max_avg_price - min_max_result.min_avg_price) / min_max_result.min_avg_price;
            double avg_volatility_multiplier = calculate_volatility_multiplier(avg_volatility, inst_config);

            double bid_beta_threshold = calculate_beta_threshold(bid_volatility, inst_config);
            double ask_beta_threshold = calculate_beta_threshold(ask_volatility, inst_config);
            double avg_beta_threshold = calculate_beta_threshold(avg_volatility, inst_config);

            BetaThreshold threshold;
            // TODO not found how to set parameter of sam
            threshold.bid_volatility = bid_volatility;
            threshold.bid_volatility_multiplier = bid_volatility_multiplier;
            threshold.bid_beta_threshold = bid_beta_threshold;
            threshold.ask_volatility = ask_volatility;
            threshold.ask_volatility_multiplier = ask_volatility_multiplier;
            threshold.ask_beta_threshold = ask_beta_threshold;
            threshold.volatility = avg_volatility;
            threshold.volatility_multiplier = avg_volatility_multiplier;
            threshold.beta_threshold = avg_beta_threshold;
            threshold.current_time_mills = now;

            shm_mng::BetaThresholdShm* shm_start = nullptr;

            if (role == TickerRole::Benchmark) {
                context.get_benchmark_beta_threshold_composite().update(base_asset, threshold);
                shm_start = context.get_shm_store_info().benchmark_beta_start;
            } else {
                context.get_follower_beta_threshold_composite().update(base_asset, threshold);
                shm_start = context.get_shm_store_info().follower_beta_start;
            }

            // store into share memory
            auto address = context.get_shm_threshold_mapping().find(base_asset);
            int update_shm = 0;
            if (address != context.get_shm_threshold_mapping().end()) {
                shm_mng::BetaThresholdShm threshold_shm;
                threshold_shm.sma = threshold.sma;
                threshold_shm.volatility = threshold.volatility;
                threshold_shm.volatility_multiplier = threshold.volatility_multiplier;
                threshold_shm.beta_threshold = threshold.beta_threshold;

                threshold_shm.bid_sma = threshold.bid_sma;
                threshold_shm.bid_volatility = threshold.bid_volatility;
                threshold_shm.bid_volatility_multiplier = threshold.bid_volatility_multiplier;
                threshold_shm.bid_beta_threshold = threshold.bid_beta_threshold;

                threshold_shm.ask_sma = threshold.ask_sma;
                threshold_shm.ask_volatility = threshold.ask_volatility;
                threshold_shm.ask_volatility_multiplier = threshold.ask_volatility_multiplier;
                threshold_shm.ask_beta_threshold = threshold.ask_beta_threshold;
                threshold_shm.time_mills = threshold.current_time_mills;


                update_shm = shm_mng::beta_shm_writer_update(shm_start, (*address).second, threshold_shm);
            }

            if (rand_value < 20) {
                // shared_ptr<shm_mng::BetaThresholdShm> shared_shm = shm_mng::beta_shm_reader_get(shm_start, (*address).second);
                // std::cout << "shm asset: " << (*shared_shm).asset << "," << (*shared_shm).bid_beta_threshold << "," << (*shared_shm).time_mills << std::endl;
                info_log("process beta threshold: role={} inst_id={} base={} bid_volatility={} bid_volatility_multiplier={} bid_beta_threshold={} ask_volatility={} ask_volatility_multiplier={} ask_beta_threshold={} volatility={} volatility_multiplier={} beta_threshold={} update_shm={}",
                    strHelper::toString(role), inst_id, base_asset, threshold.bid_volatility, threshold.bid_volatility_multiplier,
                    threshold.bid_beta_threshold, threshold.ask_volatility, threshold.ask_volatility_multiplier,
                    threshold.ask_beta_threshold, threshold.volatility, threshold.volatility_multiplier,
                    threshold.beta_threshold, update_shm);
            }
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
            
            int rnd_value = rand.randInt();

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
                if (rnd_value < 10) {
                    warn_log("can not enqueue early run calcalation: {}", ticker_info.inst_id);
                }
            }

            if (rnd_value < 20) {
                info_log("process ticker for price offset: symbol={} bid={} bid_size={} ask={} ask_size={} up_ts={}",
                    ticker_info.inst_id, ticker_info.bid_price, ticker_info.bid_volume, ticker_info.ask_price, ticker_info.ask_volume, ticker_info.update_time_millis);
            }
        }
    }

    double calculate_volatility_multiplier(double volatility, InstConfigItem &inst_config) {
        return inst_config.volatility_a + inst_config.volatility_b * volatility + inst_config.volatility_c * volatility * volatility;
    }

    double calculate_beta_threshold(double volatility_multiplier, InstConfigItem &inst_config) {
        return volatility_multiplier * inst_config.beta;
    }
}