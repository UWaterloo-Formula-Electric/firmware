#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include "../inc/source.h"
#include "../inc/state.h"
#include "../inc/decode.h"

int main (int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <can_interface>\n", argv[0]);
        return -1;
    }
    char *ifname = argv[1];
    source_t src;
    if (source_open_can(&src, ifname) == -1) {
        fprintf(stderr, "Failed to open CAN interface %s\n", ifname);
        return -1;
    }
    can_frame_t frames[32];
    state_t state = {0};
    while (1)  {
        int n = src.read(&src, frames, 32);
        if (n < 0) {
            fprintf(stderr, "Error reading from CAN interface\n");
            break;
        }
        if (n == 0) {
            usleep(1000); // lazy fix, add sleep/poll later
            continue;
        } 
        else {
            for (int i = 0; i < n; i++) {
                int ret = decode_frame(&frames[i], &state);
                if (ret < 0) {
                    fprintf(stderr, "Error processing frame #%d (%u)\n", i, frames[i].id);
                }
            }
        }

        // later: evaluate and render state data to GUI
    }
    src.close(&src);
    return 0;
}