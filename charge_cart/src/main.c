#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../inc/serial.h"

int process_bytes (const uint8_t *buf, int len) {
    for (int i = 0; i < len; i++) {
        printf("%02X ", buf[i]);
    }
    printf("\n");
    return 0;
}

int main (int argc, char *argv[]) {
    char *port = argc > 1 ? argv[1] : "error";
    if (strcmp(port, "error") == 0) {
        fprintf(stderr, "Usage: %s <port> [baudrate]\n", argv[0]);
        return -1;
    }
    int baudrate = argc > 2 ? atoi(argv[2]) : 115200;
    int fd = serial_open(port, baudrate);
    if (fd == -1) {
        fprintf(stderr, "Failed to open serial port %s at baudrate %d\n", port, baudrate);
        return -1;
    }
    uint8_t accum[1024];
    size_t accum_len = 0;
    while (1) {
        uint8_t scratch[256];
        int n = serial_read(fd, scratch, sizeof(scratch));
        if (n == -1) {
            perror("Error reading from serial port\n");
            break;
        }
        while (n > 0) {
            if (accum_len + n > sizeof(accum)) {
                perror("Accumulation buffer overflow, discarding bytes\n");
                accum_len = 0;
                continue;
            }
            memcpy(accum + accum_len, scratch, n);
            accum_len += n;
            char *result = memchr(accum, '\n', accum_len);
            if (result != NULL) {
                size_t line_len = result - (char *)accum + 1;
                process_bytes(accum, line_len);
                memmove(accum, accum + line_len, accum_len - line_len);
                accum_len -= line_len;
            }
        }
    }
    return 0;
}