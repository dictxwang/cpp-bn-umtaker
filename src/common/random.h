#ifndef _COMMON_RANDOM_H_
#define _COMMON_RANDOM_H_

#include <random>

class RandomInt32Gen {

public:
    RandomInt32Gen(int32_t low, int32_t high);
    ~RandomInt32Gen() {}

private:
    std::mt19937 gen;
    std::uniform_real_distribution<int32_t> dist;
    int32_t low;
    int32_t high;

public:
    // void init(int32_t low, int32_t high);
    int32_t randInt32();
};

#endif