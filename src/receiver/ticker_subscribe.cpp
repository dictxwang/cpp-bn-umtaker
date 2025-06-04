#include "ticker_subscribe.h"

namespace receiver {

    void start_subscribe_normal_ticker(ReceiverConfig& config, GlobalContext& context) {

        std::thread process_benchmark(process_normal_ticker_message, std::ref(config), std::ref(context), TickerRole::Benchmark);
        process_benchmark.detach();
        std::thread process_follower(process_normal_ticker_message, std::ref(config), std::ref(context), TickerRole::Follower);
        process_follower.detach();

        std::thread subscribe_benchmark(subscribe_normal_ticker, std::ref(config), std::ref(context), std::ref(context.get_benchmark_inst_ids()), TickerRole::Benchmark);
        subscribe_benchmark.detach();
        std::thread subscribe_follower(subscribe_normal_ticker, std::ref(config), std::ref(context), std::ref(context.get_follower_inst_ids()), TickerRole::Follower);
        subscribe_follower.detach();
    }

    void start_subscribe_zmq_best_ticker(ReceiverConfig& config, GlobalContext& context) {

        for (size_t i = 0; i < config.ticker_zmq_ipcs.size(); ++i) {
            std::thread subscribe_and_process(subscribe_process_zmq_best_ticker, std::ref(config), std::ref(context), i);
            subscribe_and_process.detach();
        }
    }

    void start_subscribe_udp_ticker(ReceiverConfig& config, GlobalContext& context) {
        
        for (size_t i = 0; i < config.ticker_udp_ipcs.size(); ++i) {
            std::thread subscribe_and_process(subscribe_process_udp_ticker, std::ref(config), std::ref(context), i);
            subscribe_and_process.detach();
        }
    }

    void subscribe_process_udp_ticker(ReceiverConfig& config, GlobalContext& context, size_t ipc_index) {
        
        std::string mmap_file = config.ticker_udp_ipcs[ipc_index].mmap_file;
        std::string host = config.ticker_udp_ipcs[ipc_index].host;
        int port = config.ticker_udp_ipcs[ipc_index].port;

        RandomIntGen rand;
        rand.init(0, 10000);

        // open share memory
        int mmap_fd = open(mmap_file.c_str(), O_RDONLY);
        if (mmap_fd < 0) {
            err_log("fail to open shm file {} {}", mmap_file, mmap_fd);
            return;
        }

        // create mapping for shm
        UDPMapData *g_data = (UDPMapData *)mmap(NULL, sizeof(UDPMapData), PROT_READ, MAP_SHARED, mmap_fd, 0);
        if (g_data == MAP_FAILED) {
            err_log("fail to mapping shm");
            close(mmap_fd);
            return;
        }


        while (true) {

            err_log("will connect udp ticker after 5 secs");

            // wait for a while after exception
            std::this_thread::sleep_for(std::chrono::seconds(5));

            // create udp socket
            int sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
            if (sock_fd < 0) {
                err_log("fail to create udp socket: {}", strerror(errno));
                continue;
            }

            // make subscribe request
            struct UDPBookTicker subscribe_request;
            memset(&subscribe_request, 0, sizeof(subscribe_request));
            subscribe_request.update_id = 0;
            subscribe_request.ets = 1;

            // make receiver address
            struct sockaddr_in receiver_addr;
            memset(&receiver_addr, 0, sizeof(receiver_addr));
            receiver_addr.sin_family = AF_INET;
            receiver_addr.sin_port = htons(port);
            if (inet_pton(AF_INET, host.c_str(), &receiver_addr.sin_addr) <= 0) {
                err_log("udp invalid address or address not support");
                close(sock_fd);
                continue;
            }

            try {
                // send subscribe request
                sendto(sock_fd, &subscribe_request, sizeof(subscribe_request), 0, (struct sockaddr *)&receiver_addr, sizeof(receiver_addr));

                while (true) {
                    
                    int rand_value = rand.randInt();

                    char buffer[sizeof(int)];
                    struct sockaddr_in sender_addr;
                    socklen_t addr_len = sizeof(sender_addr);
                    // receive udp ticker data
                    ssize_t recv_len = recvfrom(sock_fd, buffer, sizeof(buffer), 0, (struct sockaddr *)&sender_addr, &addr_len);
                    if (recv_len == sizeof(int)) {

                        int coin_idx = *(int *)buffer;
                        int data_idx = g_data->coin_update_idx[coin_idx] % UDP_COIN_SLOT_COUNT;
                        struct UDPBookTicker *book_ticker = &g_data->data[coin_idx][data_idx];

                        // std::cout << book_ticker->name <<"," << book_ticker->update_id <<","<< book_ticker->buy_price << "," << book_ticker->buy_num << "," << book_ticker->sell_price << ","  << book_ticker->sell_num << std::endl;
                        if (rand_value < 5) {
                            info_log("read udp shm data: name={} update_id={} bid={} bid_sz={} ask={} ask_sz={}",
                                book_ticker->name, book_ticker->update_id, book_ticker->buy_price,
                                book_ticker->buy_num, book_ticker->sell_price, book_ticker->sell_num
                            );
                        }

                        UmTickerInfo info;
                        info.inst_id = book_ticker->name;
                        info.bid_price = book_ticker->buy_price;
                        info.bid_volume = book_ticker->buy_num;
                        info.ask_price = book_ticker->sell_price;
                        info.ask_volume = book_ticker->sell_num;
                        info.avg_price = (book_ticker->buy_price) / 2;
                        info.update_time_millis = book_ticker->ets;
                        info.is_from_trade = false;

                        shm_mng::TickerInfoShm info_shm;
                        info_shm.bid_price = book_ticker->buy_price;
                        info_shm.bid_size = book_ticker->buy_num;
                        info_shm.ask_price = book_ticker->sell_price;
                        info_shm.ask_size = book_ticker->sell_num;
                        info_shm.update_id = book_ticker->update_id;
                        info_shm.update_time = book_ticker->ets;

                        int update_shm = 0;
                        bool valid_ticker = false;

                        if (str_ends_with(info.inst_id, config.benchmark_quote_asset)) {
                            valid_ticker = true;
                            // std::cout << "ticker for benchmark: " << event.symbol << std::endl;
                            context.get_benchmark_ticker_composite().update_ticker(info);

                            auto address = context.get_shm_benchmark_ticker_mapping().find(info.inst_id);
                            if (address != context.get_shm_benchmark_ticker_mapping().end()) {
                                update_shm = shm_mng::ticker_shm_writer_update(context.get_shm_store_info().benchmark_start, (*address).second, info_shm);

                                if (rand_value < 20) {
                                    std::shared_ptr<shm_mng::TickerInfoShm> ticker = shm_mng::ticker_shm_reader_get(context.get_shm_store_info().benchmark_start, (*address).second);
                                    info_log("get benchmark shm udp ticker: {}, {}, {}, {}", (*ticker).inst_id, (*ticker).bid_price, (*ticker).version_number, (*ticker).update_time);
                                }
                            }
                        } else if (str_ends_with(info.inst_id, config.follower_quote_asset)) {
                            valid_ticker = true;
                            // std::cout << "ticker for follower: " << event.symbol << std::endl;
                            context.get_follower_ticker_composite().update_ticker(info);
                        
                            auto address = context.get_shm_follower_ticker_mapping().find(info.inst_id);
                            if (address != context.get_shm_follower_ticker_mapping().end()) {
                                update_shm = shm_mng::ticker_shm_writer_update(context.get_shm_store_info().follower_start, (*address).second, info_shm);
        
                                if (rand_value < 20) {
                                    std::shared_ptr<shm_mng::TickerInfoShm> ticker = shm_mng::ticker_shm_reader_get(context.get_shm_store_info().follower_start, (*address).second);
                                    info_log("get follower shm udp ticker: {}, {}, {}, {}", (*ticker).inst_id, (*ticker).bid_price, (*ticker).version_number, (*ticker).update_time);
                                }
                            }
                        }

                        if (valid_ticker) {
                            // put info queue for price offset
                            bool result = (*context.get_ticker_info_channel()).try_enqueue(info);
                            if (!result) {
                                warn_log("can not enqueue ticker info: {}", info.inst_id);
                            }
                        }

                        if (rand_value < 20) {
                            info_log("process udp ticker: symbol={} bid={} bid_size={} ask={} ask_size={} up_ts={} update_shm={}",
                                info.inst_id, info.bid_price, info.bid_volume, info.ask_price, info.ask_volume, info.update_time_millis, update_shm);
                        }
                    }
                }
            } catch (std::exception &exp) {
                err_log("exception occur while udp comminication: {}", std::string(exp.what()));
            }

            if (sock_fd > 0) {
                close(sock_fd);
            }
        }
    }

    void process_normal_ticker_message(ReceiverConfig& config, GlobalContext &context, TickerRole role) {
        moodycamel::ConcurrentQueue<string> *channel;
        if (role == TickerRole::Benchmark) {
            channel = context.get_benchmark_ticker_channel();
        } else {
            channel = context.get_follower_ticker_channel();
        }

        RandomIntGen rand;
        rand.init(0, 10000);

        while (true) {
            std::string messageJson;
            while (!channel->try_dequeue(messageJson)) {
                // Retry if the queue is empty
            }

            try {
                // process message
                Json::Value json_result;
                Json::Reader reader;
                json_result.clear();
                reader.parse(messageJson.c_str(), json_result);

                if (json_result.isMember("data")) {
                    json_result = json_result["data"];
                }
                binance::WsFuturesBookTickerEvent event = binance::convertJsonToWsFuturesBookTickerEvent(json_result);

                UmTickerInfo info;
                info.inst_id = event.symbol;
                info.bid_price = event.bestBidPrice;
                info.bid_volume = event.bestBidQty;
                info.ask_price = event.bestAskPrice;
                info.ask_volume = event.bestAskQty;
                info.avg_price = (event.bestBidPrice + event.bestAskPrice) / 2;
                info.update_time_millis = event.eventTime;
                info.is_from_trade = false;

                shm_mng::TickerInfoShm info_shm;
                info_shm.bid_price = info.bid_price;
                info_shm.bid_size = info.bid_volume;
                info_shm.ask_price = info.ask_price;
                info_shm.ask_size = info.ask_volume;
                info_shm.update_id = 0;
                info_shm.update_time = info.update_time_millis;

                int rand_value = rand.randInt();
                int update_shm = 0;
                if (role == TickerRole::Benchmark) {
                    // std::cout << "ticker for benchmark: " << event.symbol << std::endl;
                    context.get_benchmark_ticker_composite().update_ticker(info);
                    auto address = context.get_shm_benchmark_ticker_mapping().find(info.inst_id);
                    if (address != context.get_shm_benchmark_ticker_mapping().end()) {
                        update_shm = shm_mng::ticker_shm_writer_update(context.get_shm_store_info().benchmark_start, (*address).second, info_shm);

                        if (rand_value < 20) {
                            std::shared_ptr<shm_mng::TickerInfoShm> ticker = shm_mng::ticker_shm_reader_get(context.get_shm_store_info().benchmark_start, (*address).second);
                            info_log("get benchmark shm normal ticker: {}, {}, {}, {}", (*ticker).inst_id, (*ticker).bid_price, (*ticker).version_number, (*ticker).update_time);
                        }
                    }
                } else {
                    // std::cout << "ticker for follower: " << event.symbol << std::endl;
                    context.get_follower_ticker_composite().update_ticker(info);
                    auto address = context.get_shm_follower_ticker_mapping().find(info.inst_id);
                    if (address != context.get_shm_follower_ticker_mapping().end()) {
                        update_shm = shm_mng::ticker_shm_writer_update(context.get_shm_store_info().follower_start, (*address).second, info_shm);

                        if (rand_value < 20) {
                            std::shared_ptr<shm_mng::TickerInfoShm> ticker = shm_mng::ticker_shm_reader_get(context.get_shm_store_info().follower_start, (*address).second);
                            info_log("get follower shm normal ticker: {}, {}, {}, {}", (*ticker).inst_id, (*ticker).bid_price, (*ticker).version_number, (*ticker).update_time);
                        }
                    }
                }

                // put info queue for price offset
                bool result = (*context.get_ticker_info_channel()).try_enqueue(info);
                if (!result) {
                    warn_log("can not enqueue ticker info: {}", messageJson);
                }

                result = (*context.get_beta_calculation_channel()).try_enqueue(info.inst_id);
                if (!result) {
                    if (rand_value < 10) {
                        warn_log("can not enqueue beta calculation: {}", info.inst_id);
                    }
                }

                if (rand_value < 20) {
                    info_log("process normal ticker: role={} symbol={} bid={} bid_size={} ask={} ask_size={} up_ts={} update_shm={}",
                        strHelper::toString(role), info.inst_id, info.bid_price, info.bid_volume, info.ask_price, info.ask_volume, info.update_time_millis, update_shm);
                }
            } catch (std::exception &exp) {
                err_log("fail to process normal ticker message: {}", std::string( exp.what()));
            }
        }
    }

    void subscribe_normal_ticker(ReceiverConfig &config, GlobalContext& context, vector<string> &inst_ids, TickerRole role) {


        binance::BinanceFuturesWsClient futuresWsClient;

        if (config.normal_ticker_local_ip != "") {
            futuresWsClient.setLocalIP(config.normal_ticker_local_ip);
        }

        futuresWsClient.initBookTickerV1(config.normal_ticker_use_intranet, false);

        if (role == TickerRole::Benchmark) {
            futuresWsClient.setMessageChannel(context.get_benchmark_ticker_channel());
        } else {
            futuresWsClient.setMessageChannel(context.get_follower_ticker_channel());
        }

        while (true) {
            std::pair<bool, string> result;
            try {
                result = futuresWsClient.startBookTickerV1(inst_ids);
            } catch (std::exception &exp) {
                err_log("error occur while book ticker: {}", std::string(exp.what()));
            }

            err_log("stop book ticker subscribe: {}", result.second);

            // wait for a while after exception
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
    }

    void subscribe_process_zmq_best_ticker(ReceiverConfig& config, GlobalContext& context, size_t ipc_index) {

        while (true) {
            ZMQClient zmq_client(ZMQ_SUB);
            try {
                zmq_client.SubscriberConnect(config.ticker_zmq_ipcs[ipc_index]);
            } catch (std::exception &exp) {
                err_log("error occur while zmq subscribe connection: {}", std::string(exp.what()));
            }

            RandomIntGen rand;
            rand.init(0, 10000);

            while (true) {
                try {
                    std::string message = zmq_client.Receive();
                    besttick::TickerInfo event;
                    event.ParseFromString(message);

                    UmTickerInfo info;
                    info.inst_id = event.instid();
                    info.bid_price = event.bestbid();
                    info.bid_volume = event.bidsz();
                    info.ask_price = event.bestask();
                    info.ask_volume = event.asksz();
                    info.avg_price = (event.bestbid() + event.bestask()) / 2;
                    info.update_time_millis = event.eventts();
                    info.is_from_trade = false;

                    shm_mng::TickerInfoShm info_shm;
                    info_shm.bid_price = info.bid_price;
                    info_shm.bid_size = info.bid_volume;
                    info_shm.ask_price = info.ask_price;
                    info_shm.ask_size = info.ask_volume;
                    info_shm.update_id = event.updateid();
                    info_shm.update_time = info.update_time_millis;

                    int rand_value = rand.randInt();
                    int update_shm = 0;
                    bool valid_ticker = false;

                    if (str_ends_with(info.inst_id, config.benchmark_quote_asset)) {
                        valid_ticker = true;
                        // std::cout << "ticker for benchmark: " << event.symbol << std::endl;
                        context.get_benchmark_ticker_composite().update_ticker(info);

                        auto address = context.get_shm_benchmark_ticker_mapping().find(info.inst_id);
                        if (address != context.get_shm_benchmark_ticker_mapping().end()) {
                            update_shm = shm_mng::ticker_shm_writer_update(context.get_shm_store_info().benchmark_start, (*address).second, info_shm);

                            if (rand_value < 20) {
                                std::shared_ptr<shm_mng::TickerInfoShm> ticker = shm_mng::ticker_shm_reader_get(context.get_shm_store_info().benchmark_start, (*address).second);
                                info_log("get benchmark shm zmq ticker: {}, {}, {}, {}", (*ticker).inst_id, (*ticker).bid_price, (*ticker).version_number, (*ticker).update_time);
                            }
                        }
                    } else if (str_ends_with(info.inst_id, config.follower_quote_asset)) {
                        valid_ticker = true;
                        // std::cout << "ticker for follower: " << event.symbol << std::endl;
                        context.get_follower_ticker_composite().update_ticker(info);
                       
                        auto address = context.get_shm_follower_ticker_mapping().find(info.inst_id);
                        if (address != context.get_shm_follower_ticker_mapping().end()) {
                            update_shm = shm_mng::ticker_shm_writer_update(context.get_shm_store_info().follower_start, (*address).second, info_shm);
    
                            if (rand_value < 20) {
                                std::shared_ptr<shm_mng::TickerInfoShm> ticker = shm_mng::ticker_shm_reader_get(context.get_shm_store_info().follower_start, (*address).second);
                                info_log("get follower shm zmq ticker: {}, {}, {}, {}", (*ticker).inst_id, (*ticker).bid_price, (*ticker).version_number, (*ticker).update_time);
                            }
                        }
                    }

                    if (valid_ticker) {
                        // put info queue for price offset
                        bool result = (*context.get_ticker_info_channel()).try_enqueue(info);
                        if (!result) {
                            warn_log("can not enqueue ticker info: {}", info.inst_id);
                        }
                    }

                    if (rand_value < 20) {
                        info_log("process zmq best ticker: symbol={} bid={} bid_size={} ask={} ask_size={} up_ts={} update_shm={}",
                            info.inst_id, info.bid_price, info.bid_volume, info.ask_price, info.ask_volume, info.update_time_millis, update_shm);
                    }
                    
                } catch (std::exception &exp) {
                    err_log("error occur while parse zmq message");
                    break;
                }

                err_log("will re-subscribe zmq ticker after 5 secs");

                // wait for a while after exception
                std::this_thread::sleep_for(std::chrono::seconds(5));
            }
        }
    }
}