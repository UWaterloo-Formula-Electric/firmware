#include "mathUtils.h"
#include <string.h>

float min(float a, float b) {
    if (a < b) {
        return a;
    } else {
        return b;
    }
}

float max(float a, float b) {
    if (a > b) {
        return a;
    } else {
        return b;
    }
}

float clip(float in, float low, float high) {
    if (in < low)
        return low;
    if (in > high)
        return high;
    return in;
}

int map_range(int in, int low, int high, int low_out, int high_out) {
    if (in < low) {
        in = low;
    } else if (in > high) {
        in = high;
    }
    int in_range = high - low;
    int out_range = high_out - low_out;

    return (float)(in - low) * out_range / in_range + low_out;
}

float map_range_float(float in, float low, float high, float low_out, float high_out) {
    if (in < low) {
        return low_out;
    } else if (in > high) {
        return high_out;
    }
    const float in_range = high - low;
    const float out_range = high_out - low_out;

    return (in - low) * out_range / in_range + low_out;
}

float get_median(const float *arr, size_t size) {
    // Sort a copy, callers pass ring buffers whose order must be preserved
    float sorted[size];
    memcpy(sorted, arr, sizeof(sorted));
    for (size_t i = 0; i < size - 1; i++) {
        for (size_t j = i + 1; j < size; j++) {
            if (sorted[i] > sorted[j]) {
                float temp = sorted[i];
                sorted[i] = sorted[j];
                sorted[j] = temp;
            }
        }
    }
    // Return the median value
    if (size % 2 == 0) {
        // If even, return the average of the two middle values
        return (sorted[size / 2 - 1] + sorted[size / 2]) / 2;
    }

    return sorted[size / 2];
}