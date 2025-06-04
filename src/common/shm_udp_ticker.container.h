#ifndef _COMMON_UDP_TICKER_CONTAINER_H_
#define _COMMON_UDP_TICKER_CONTAINER_H_

#include <iostream>

using namespace std;

#define UDP_MAX_COIN_COUNT 512  // max coin store in share-memory
#define UDP_COIN_SLOT_COUNT 32  // slots for one coin

struct UDPTickerIPC {
    string mmap_file;
    string host;
    int port;
};

struct UDPBookTicker {
    uint64_t update_id;
    uint64_t ets;
    float buy_price;
    float buy_num;
    float sell_price;
    float sell_num;
    char name[16];
};

struct UDPMapData {
    int coin_count;
    char coin_name[UDP_MAX_COIN_COUNT][16];
    int coin_update_idx[UDP_MAX_COIN_COUNT];
    struct UDPBookTicker data[UDP_MAX_COIN_COUNT][UDP_COIN_SLOT_COUNT];
};

#endif
