#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../inc/source.h"
#include "../inc/state.h"
#include "../inc/decode.h"
#include "../inc/render.h"
#include "../inc/util.h"
#include "../inc/evaluate.h"
#include <raylib.h>

int main (int argc, char *argv[]) {
    evaluation_t evaluation = {0};
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
    SetConfigFlags(FLAG_FULLSCREEN_MODE);
    InitWindow(0, 0, "Charge Cart"); // 0x0 = native display size
    SetTargetFPS(33);
    while (!WindowShouldClose())  {
        int wait_ret = src.wait(&src, 100);
        if (wait_ret < 0) {
            fprintf(stderr, "Error waiting for CAN interface\n");
            break;
        }
        int n, rounds=0;
        do { // drain the socket each frame
            n = src.read(&src, frames, 32);
            if (n < 0) {
                fprintf(stderr, "Error reading from CAN interface\n");
                break;
            }
            for (int i = 0; i < n; i++) {
                int ret = decode_frame(&frames[i], &state);
                if (ret < 0) {
                    fprintf(stderr, "Error processing frame #%d (%u)\n", i, frames[i].id);
                }
            }
        } while (n == 32 && ++rounds < 10);
        evaluate_state(&state, &evaluation, now_ms());

        BeginDrawing();
        render(&state, &evaluation);
        EndDrawing();
    }
    CloseWindow();
    src.close(&src);
    return 0;
}