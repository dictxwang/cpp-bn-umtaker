#include "api.h"
#include "binancecpp/util/string_helper.h"
#include <stdexcept>

namespace tgbot {
    void TgApi::init(const std::string& url, const std::string& token) {
        this->_url = url;
        this->_token = token;
    }

    std::pair<int, std::string> TgApi::send_message(int64_t chat_id, const std::string& text) {

        std::vector<std::pair<std::string, std::string>> body_params;
        body_params.push_back(std::pair<std::string, std::string>("chat_id", strHelper::toString(chat_id)));
        body_params.push_back(std::pair<std::string, std::string>("text", text));

        std::string call_result;
        std::vector<std::string> emtpy;
        std::vector<std::pair<std::string, std::string>> emtpy_params;

        sendRequest("sendMessage", "POST", call_result, emtpy, emtpy_params, body_params);

        // TODO parse result, set frequence of send message
        //  {"ok":true,"result":{"message_id":50347,"from":{"id":7124818401,"is_bot":true,"first_name":"Watchdog007","username":"my_oss_bot"},"chat":{"id":-4958151325,"title":"Binance UM Taker","type":"group","all_members_are_administrators":true,"accepted_gift_types":{"unlimited_gifts":false,"limited_gifts":false,"unique_gifts":false,"premium_subscription":false}},"date":1748884816,"text":"123456"}}
        std::cout << "tg: " << call_result << std::endl;
    }

    CURLcode TgApi::sendRequest(const std::string& api_method, const std::string& action,  std::string& call_result, std::vector <std::string> &extra_header, std::vector<std::pair<std::string, std::string>> &query_params, std::vector<std::pair<std::string, std::string>> &form_params) {
        std::string url(this->_url);
        url += "/bot";
        url += _token;
        url += "/";
        url += api_method;

        std::string delimiter_join = "&";
        std::string queryString = "";
        std::string formString = "";

        CURL* curl = curl_easy_init();
        CURLcode call_code;

        if (!query_params.empty()) {
            std::vector<std::string> query_param_list;
            for (std::pair<std::string, std::string> p : query_params) {
                query_param_list.push_back(p.first + "=" + curl_easy_escape(curl, p.second.c_str(), 0));
            }
            queryString = strHelper::joinStrings(query_param_list, delimiter_join);
        }

        if (!form_params.empty()) {
            std::vector<std::string> form_param_list;
            for (std::pair<std::string, std::string> p : form_params) {
                form_param_list.push_back(p.first + "=" + curl_easy_escape(curl, p.second.c_str(), 0));
            }
            formString = strHelper::joinStrings(form_param_list, delimiter_join);
            extra_header.push_back("Content-Type: application/x-www-form-urlencoded");
        }

        std::string fullUrl = url;
        if (queryString.size() > 0) {
            fullUrl.append("?").append(queryString);
        }

        curl_easy_setopt(curl, CURLOPT_URL, fullUrl.c_str() );
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &call_result );
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, false);

        if (extra_header.size() > 0 ) {
            
            struct curl_slist *chunk = nullptr;
            for ( size_t i = 0; i < extra_header.size(); ++i ) {
                chunk = curl_slist_append(chunk, extra_header[i].c_str() );
            }
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);
        }

        if ( formString.size() > 0 || action == "POST" || action == "PUT" || action == "DELETE" ) {

            if ( action == "PUT" || action == "DELETE" ) {
                curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, action.c_str() );
            }
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, formString.c_str() );
        }

        call_code = curl_easy_perform(curl);
        
        if (curl != nullptr) {
            curl_easy_cleanup(curl);
        }

        return call_code;
    }

    static size_t curl_write_callback(void* contents, size_t size, size_t nmemb, std::string* userData) {
        size_t totalSize = size * nmemb;
        userData->append(static_cast<char*>(contents), totalSize);
        return totalSize;
    }
}