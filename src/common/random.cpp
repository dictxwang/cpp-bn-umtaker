#include "random.h"

void RandomIntGen::init(int min, int max) {
    this->min = min;
    this->max = max;

       // Thread-local random engine
       thread_local std::mt19937 generator(std::random_device{}()); // Seed per thread
       // Uniform integer distribution
       std::uniform_int_distribution<int> distribution(min, max);


    this->generator = generator;
    this->distribution = distribution;
}

int RandomIntGen::randInt() {
    return distribution(generator);
}