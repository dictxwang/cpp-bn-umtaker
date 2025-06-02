#ifndef _TGBOG_API_H_
#define _TGBOG_API_H_

#include <iostream>
#include <cstdint>
#include <vector>
#include <curl/curl.h>

namespace tgbot {

    static size_t curl_write_callback(void* contents, size_t size, size_t nmemb, std::string* userData);

    class TgApi {
    public:
        TgApi() {}
        ~TgApi() {}

        void init(const std::string& url, const std::string& token);
        std::pair<int, std::string> send_message(int64_t chat_id, const std::string& text);
    
    private:
        CURLcode sendRequest(const std::string& api_method, const std::string& action, std::string& call_result, std::vector <std::string> &extra_header, std::vector<std::pair<std::string, std::string>> &query_params, std::vector<std::pair<std::string, std::string>> &form_params);
    
    protected:
        std::string _token;
        std::string _url;
    };

}
#endif