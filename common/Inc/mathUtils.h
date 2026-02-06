#ifndef MATH_UTILS_H
#define MATH_UTILS_H

#include <stddef.h>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

float map_range_float(float in, float low, float high, float low_out, float high_out);
int map_range(int in, int low, int high, int low_out, int high_out);
float min(float a, float b);
float max(float a, float b);
float clip(float in, float low, float high);
float get_median(float *arr, size_t size);


#endif /* end of include guard: MATH_UTILS_H */
