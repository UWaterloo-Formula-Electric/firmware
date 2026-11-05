#include <stdint.h>
#include "state.h"

int can_open(const char *ifname);
int can_read(int fd, can_frame_t *out, int max);
int can_close(int fd);