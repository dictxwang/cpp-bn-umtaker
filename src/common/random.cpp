#include "random.h"

RandomInt32Gen::RandomInt32Gen(int32_t low, int32_t high) {
    this->low = low;
    this->high = high;

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<int32_t> dist(low, high);

    this->dist = dist;
    this->gen = gen;
}

int32_t RandomInt32Gen::randInt32() {
    return dist(gen);
}