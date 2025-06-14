#include "service_manager.h"

using namespace std;

namespace trader {
    
    void start_best_service_management(TraderConfig &config, GlobalContext &context) {

        if (config.trading_use_best_path) {
            thread polling_thread(polling_best_path_processor, ref(config), ref(context));
            polling_thread.detach();
            info_log("start polling best path api thread");

            thread sub_zmq_thread(subscribe_best_path_processor, ref(config), ref(context));
            sub_zmq_thread.detach();
            info_log("start subscribe best path zmq thread");

            thread zookeeper_thread(service_zookeeper_processor, ref(config), ref(context));
            zookeeper_thread.detach();
            info_log("start best path zookeeper thread");
        } else {
            warn_log("not use best path service for trading");
        }
    }

    void service_zookeeper_processor(TraderConfig &config, GlobalContext &context) {

        while (true) {
            this_thread::sleep_for(chrono::seconds(60));
            info_log("server zookeeper start work");
            vector<string> all_ip_pairs = context.get_order_service_manager().get_all_service_ip_pairs();
            set<string> in_used_ip_pairs = context.get_order_service_manager().get_in_use_ip_pairs();
            vector<string> in_used_ip_pair_list(in_used_ip_pairs.begin(), in_used_ip_pairs.end());

            info_log("find all ip pairs for services {}", strHelper::joinStrings(all_ip_pairs, ","));
            info_log("find inused ip pairs for services {}", strHelper::joinStrings(in_used_ip_pair_list, ","));

            // find ip_pair of unused services
            vector<string> unused_ip_pairs;
            for (string ip_pair : all_ip_pairs) {
                if (in_used_ip_pairs.find(ip_pair) == in_used_ip_pairs.end()) {
                    unused_ip_pairs.push_back(ip_pair);
                }
            }

            info_log("find unsed ip pairs for services {}", strHelper::joinStrings(unused_ip_pairs, ","));
           
            for (string ip_pair : unused_ip_pairs) {
                context.get_order_service_manager().stop_and_remove_service(ip_pair);
            }
        }
    }

    void polling_best_path_processor(TraderConfig &config, GlobalContext &context) {

        this_thread::sleep_for(chrono::seconds(5));
        while (true) {
            vector<BestPathInfo> bestpath_list = simple_call_best_path(config);
            info_log("get best path result from api, size is {}", bestpath_list.size());
            uint64_t now = binance::get_current_ms_epoch();

            for (BestPathInfo info : bestpath_list) {
                info_log("best path from api: {} {} {}->{} {} {}", info.action, info.symbol, info.local_ip, info.remote_ip, info.best_cost, info.update_time_millis);
                
                if (!is_valid_best_path(context, info)) {
                    continue;;
                }

                shared_ptr<WsClientWrapper> new_wrapper = make_shared<WsClientWrapper>();
                context.get_order_service_manager().update_best_service(config, info.symbol, info.local_ip, info.remote_ip, new_wrapper, context.get_order_channel());
                this_thread::sleep_for(chrono::seconds(1));
            }

            this_thread::sleep_for(chrono::minutes(5));
        }
    }
    
    void subscribe_best_path_processor(TraderConfig &config, GlobalContext &context) {

        this_thread::sleep_for(chrono::seconds(5));
        while (true) {

            ZMQClient zmq_client(ZMQ_SUB);
            try {
                zmq_client.SubscriberConnect(config.best_path_zmq_ipc);
            } catch (std::exception &exp) {
                err_log("error occur while zmq subscribe connection: {}", std::string(exp.what()));
            }

            while (true) {
                try {
                    std::string message = zmq_client.Receive();
                    info_log("get best path zmq message: {}", message);

                    Json::Value json_result;
                    Json::Reader reader;
                    json_result.clear();
                    reader.parse(message, json_result);

                    BestPathInfo info;
                    info.symbol = json_result["symbol"].asString();
                    info.action = json_result["action"].asString();
                    info.local_ip = json_result["local_ip"].asString();
                    info.remote_ip = json_result["remote_ip"].asString();
                    info.best_cost = json_result["best_cost"].asUInt64();
                    info.update_time_millis = json_result["update_time_millis"].asUInt64();
                
                    if (!is_valid_best_path(context, info)) {
                        continue;;
                    }
                    
                    shared_ptr<WsClientWrapper> new_wrapper = make_shared<WsClientWrapper>();
                    context.get_order_service_manager().update_best_service(config, info.symbol, info.local_ip, info.remote_ip, new_wrapper, context.get_order_channel());
                    
                } catch (exception &exp) {
                    err_log("error occur while parse zmq message");
                    break;
                }
            }

            err_log("will re-subscribe zmq bestpath after 5 secs");

            // wait for a while after exception
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
    }

    bool is_valid_best_path(GlobalContext &context, const BestPathInfo &info) {
        if (context.get_follower_inst_id_set().find(info.symbol) == context.get_follower_inst_id_set().end()) {
            warn_log("best path not support symbol {}", info.symbol);
            return false;
        }
        if (info.action != BEST_PATH_ACTION_NEW) {
            warn_log("best path not support action {}", info.action);
            return false;
        }
        if (info.local_ip.size() == 0 || info.remote_ip.size() == 0 || info.best_cost <= 0) {
            warn_log("best path has invalid values");
            return false;
        }

        uint64_t now = binance::get_current_ms_epoch();
        if (info.update_time_millis + 3600*1000 < now) {
            // expired info
            warn_log("best path has expired {} {}", info.update_time_millis, now);
            return false;
        }

        return true;
    }

    vector<BestPathInfo> simple_call_best_path(TraderConfig &config) {
        CURL* curl = curl_easy_init();
        CURLcode call_code;

        string call_result;
        curl_easy_setopt(curl, CURLOPT_URL, config.best_path_rest_url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &call_result );
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, false);

        call_code = curl_easy_perform(curl);
        
        if (curl != nullptr) {
            curl_easy_cleanup(curl);
        }

        vector<BestPathInfo> result;

        if (call_code != CURLE_OK) {
            err_log("fail to call best path api: {}", strHelper::toString(call_code));
        } else {
            try {
                Json::Value json_result;
                Json::Reader reader;
                json_result.clear();
                reader.parse(call_result , json_result);

                info_log("best path api response: {}", call_result);

                if (json_result.isArray()) {
                    for (int i = 0; i < json_result.size(); i++) {
                        BestPathInfo info;
                        info.symbol = json_result[i]["symbol"].asString();
                        info.action = json_result[i]["action"].asString();
                        info.local_ip = json_result[i]["local_ip"].asString();
                        info.remote_ip = json_result[i]["remote_ip"].asString();
                        info.best_cost = json_result[i]["best_cost"].asUInt64();
                        info.update_time_millis = json_result[i]["update_time_millis"].asUInt64();

                        result.push_back(info);
                    }
                }
            } catch (exception &exp) {
                err_log("fail to parse call best path api response: {}", string(exp.what()));
            }
        }

        return result;
    }
}