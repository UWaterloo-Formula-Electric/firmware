#pragma once
#include <stdint.h>
#include "state.h"

typedef struct source {
    int (*read)(struct source *self, can_frame_t *out, int max);
    int (*close)(struct source *self);
    void *ctx;
} source_t;

int source_open_can(source_t *src, const char *ifname);
