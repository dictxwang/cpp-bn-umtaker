#ifndef _CONFIG_CONFIG_H_
#define _CONFIG_CONFIG_H_   

#include <fstream>
#include <iostream>
#include <string>
#include <cstring>
#include <stdexcept>
#include "binancecpp/json/json.h"

using std::string;

class Config
{
public:
    Config() {};
    virtual ~Config() {}

protected:
    bool load_config(const char* inputfile);

public:
    string logger_name;
    int logger_level;
    int logger_max_files;
    string logger_file_path;

protected:
    Json::Value doc_;
};
#endif