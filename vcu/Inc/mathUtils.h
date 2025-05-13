#ifndef MATH_UTILS_H

#define MATH_UTILS_H

#include "bsp.h"

float map_range_float(float in, float low, float high, float low_out, float high_out);
int map_range(int in, int low, int high, int low_out, int high_out);
float min(float a, float b);

#endif /* end of include guard: MATH_UTILS_H */
