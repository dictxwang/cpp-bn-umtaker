#include "main_config.h"

bool MainConfig::loadMainConfig(const char* inputfile) {
    bool loadFileResult = Config::load_config(inputfile);
    if (!loadFileResult) {
        return false;
    }

    // Parse own configuration properties
    this->api_key_hmac = this->doc_["api_key_hmac"].asString();
    this->secret_key_hmac = this->doc_["secret_key_hmac"].asString();
    this->api_key_ed25519 = this->doc_["api_key_ed25519"].asString();
    this->secret_key_ed25519 = this->doc_["secret_key_ed25519"].asString();
    this->zmq_ipc = this->doc_["zmq_ipc"].asString();

    return true;
}