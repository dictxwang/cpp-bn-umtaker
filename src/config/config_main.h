#ifndef _CONFIG_CONFIG_MAIN_H_
#define _CONFIG_CONFIG_MAIN_H_

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
    std::string api_key_ed25519;
    std::string secret_key_ed25519;
    std::string zmq_ipc;
};
#endif  