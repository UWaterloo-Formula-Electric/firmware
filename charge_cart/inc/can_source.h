#include <stdint.h>

typedef struct {
    uint32_t id;
    uint8_t len;
    uint8_t data[8];
    uint64_t rx_ms;
} can_frame_t;


int can_open(const char *ifname);
int can_read(int fd, can_frame_t *out, int max);
int can_close(int fd);