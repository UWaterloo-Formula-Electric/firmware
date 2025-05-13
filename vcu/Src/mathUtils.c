#include "mathUtils.h"

float min(float a, float b)
{
    if (a < b) {
        return a;
    } else {
        return b;
    }
}

int map_range(int in, int low, int high, int low_out, int high_out) {
    if (in < low) {
        in = low;
    } else if (in > high) {
        in = high;
    }
    int in_range = high - low;
    int out_range = high_out - low_out;

    return (in - low) * out_range / in_range + low_out;
}

float map_range_float(float in, float low, float high, float low_out, float high_out)
{
    if (in < low) {
        in = low;
    } else if (in > high) {
        in = high;
    }
    const float in_range = high - low;
    const float out_range = high_out - low_out;

    return (((in - low) * out_range) / (in_range + low_out));
}
