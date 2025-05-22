#ifndef _COMMON_RANDOM_H_
#define _COMMON_RANDOM_H_

#include <random>

class RandomIntGen {

public:
    RandomIntGen() {}
    ~RandomIntGen() {}

private:
    std::mt19937 generator;
    std::uniform_int_distribution<int> distribution;
    int min;
    int max;

public:
    void init(int low, int high);
    int randInt();
};

#endif