#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include "../inc/serial.h"
#include "../inc/can_source.h"

int process_frame (const can_frame_t *frame) {
    printf("ID: %08X, LEN: %d, DATA: ", frame->id, frame->len);
    for (int i = 0; i < frame->len; i++) {
        printf("%02X ", frame->data[i]);
    }
    printf("\n");
    if (0) {
        //error checking later
        return -1;
    }
    return 0;
}

int main (int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <can_interface>\n", argv[0]);
        return -1;
    }
    char *ifname = argv[1];
    int fd = can_open(ifname);
    if (fd == -1) {
        fprintf(stderr, "Failed to open CAN interface %s\n", ifname);
        return -1;
    }
    can_frame_t frames[32];
    while (1)  {
        int n = can_read(fd, frames, 32);
        if (n < 0) {
            fprintf(stderr, "Error reading from CAN interface\n");
            break;
        }
        if (n == 0) {
            usleep(1000); // lazy fix, add sleep later
            continue;
        } 
        else {
            for (int i = 0; i < n; i++) {
                int ret = process_frame(&frames[i]);
                if (ret < 0) {
                    fprintf(stderr, "Error processing frame #%d (%u)\n", i, frames[i].id);
                }
            }
        }
    }
    can_close(fd);
    return 0;
}