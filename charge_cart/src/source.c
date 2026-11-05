#include <stdint.h>
#include "../inc/source.h"
#include "../inc/can_source.h"

static int can_source_read (source_t *self, can_frame_t *out, int max) {
    return can_read((int)(intptr_t)self->ctx, out, max);
}

static int can_source_close (source_t *self) {
    return can_close((int)(intptr_t)self->ctx);
}

int source_open_can (source_t *src, const char *ifname) {
    int fd = can_open(ifname);
    if (fd == -1) {
        return -1;
    }
    src->read = can_source_read;
    src->close = can_source_close;
    src->ctx = (void *)(intptr_t)fd;
    return 0;
}
