#ifndef _CONFIG_MAIN_CONFIG_H_
#define _CONFIG_MAIN_CONFIG_H_

#include "config.h"

class MainConfig : public Config
{
public:
    MainConfig() {}
    ~MainConfig() {}

public:
    bool loadMainConfig(const char* inputfile);

public:
    std::string api_key_hmac;
    std::string secret_key_hmac;
    bool rest_use_intranet;
    std::string rest_local_ip;

    std::string api_key_ed25519;
    std::string secret_key_ed25519;
    std::string zmq_ipc;
};
#endif  