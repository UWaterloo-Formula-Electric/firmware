#pragma once
#include <stdint.h>
#include <time.h>

// monotonic, so rx timestamps and staleness checks don't jump with wall clock changes
static inline uint64_t now_ms (void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}
