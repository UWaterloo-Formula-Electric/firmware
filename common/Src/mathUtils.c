#include "mathUtils.h"

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

float map_range_float(float in, float low, float high, float low_out, float high_out) {
    if (in < low) {
        return low_out;
    } else if (in > high) {
        return high_out;
    }
    const float in_range = high - low;
    const float out_range = high_out - low_out;

    return (((in - low) * out_range) / (in_range + low_out));
}

float get_median(float *arr, size_t size) {
    // Sort the array
    for (size_t i = 0; i < size - 1; i++) {
        for (size_t j = i + 1; j < size; j++) {
            if (arr[i] > arr[j]) {
                float temp = arr[i];
                arr[i] = arr[j];
                arr[j] = temp;
            }
        }
    }
    // Return the median value
    if (size % 2 == 0) {
        // If even, return the average of the two middle values
        return (arr[size / 2 - 1] + arr[size / 2]) / 2;
    }

    return arr[size / 2];
}