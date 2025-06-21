#ifndef _COMMON_CONSTANT_H_
#define _COMMON_CONSTANT_H_

#include <string>

using namespace std;

enum TickerRole {
    Benchmark,
    Follower,
};

const string Early_Run_Calculation_Type_Average = "average";
const string Early_Run_Calculation_Type_Median = "median";

#endif
