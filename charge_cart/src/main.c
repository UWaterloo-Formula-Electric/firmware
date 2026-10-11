#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include "../inc/serial.h"
#include "../inc/can_source.h"
#include "../inc/state.h"

int process_frame (const can_frame_t *frame, state_t *state) {
    uint64_t raw = 0;
    for (int i = 7; i >= 0; i--)
        raw = (raw << 8) | (i < frame->len ? frame->data[i] : 0);

    switch (frame->id) {
        case BMU_CELLVOLTAGE: { // BMU_CellVoltage
            uint8_t mux = raw & 0xFF;
            float scale = 0.0001f;
            if (mux*3 < NUM_VOLTAGE_CELLS) {
                state->voltage[mux*3] = ((raw >> 8) & 0xFFFF) * scale;
                state->cell_rx_ms[mux*3] = frame->rx_ms;
            }
            if (mux*3 + 1 < NUM_VOLTAGE_CELLS) {
                state->voltage[mux*3 + 1] = ((raw >> 24) & 0xFFFF) * scale;
                state->cell_rx_ms[mux*3 + 1] = frame->rx_ms;
            }
            if (mux*3 + 2 < NUM_VOLTAGE_CELLS) {
                state->voltage[mux*3 + 2] = ((raw >> 40) & 0xFFFF) * scale;
                state->cell_rx_ms[mux*3 + 2] = frame->rx_ms;
            }
            break;
        }
        case BMU_CELLVOLTAGE_ADJUSTED: { // BMU_CellVoltage_Adjusted
            uint8_t mux = raw & 0xFF;
            float scale = 0.0001f;
            if (mux*3 < NUM_VOLTAGE_CELLS) {
                state->voltageAdjusted[mux*3] = ((raw >> 8) & 0xFFFF) * scale;
                state->cell_rx_ms[mux*3] = frame->rx_ms;
            }
            if (mux*3 + 1 < NUM_VOLTAGE_CELLS) {
                state->voltageAdjusted[mux*3 + 1] = ((raw >> 24) & 0xFFFF) * scale;
                state->cell_rx_ms[mux*3 + 1] = frame->rx_ms;
            }
            if (mux*3 + 2 < NUM_VOLTAGE_CELLS) {
                state->voltageAdjusted[mux*3 + 2] = ((raw >> 40) & 0xFFFF) * scale;
                state->cell_rx_ms[mux*3 + 2] = frame->rx_ms;
            }
            break;
        }

        case BMU_CHANNELTEMP: { // BMU_ChannelTemp
            uint8_t mux = raw & 0xFF;
            float offset = -100;
            float scale = 0.01f;
            if (mux*3 < NUM_TEMP_CELLS) {
                state->temperature[mux*3] = ((raw >> 8) & 0xFFFF) * scale + offset;
                state->temp_rx_ms[mux*3] = frame->rx_ms;
            }
            if (mux*3 + 1 < NUM_TEMP_CELLS) {
                state->temperature[mux*3 + 1] = ((raw >> 24) & 0xFFFF) * scale + offset;
                state->temp_rx_ms[mux*3 + 1] = frame->rx_ms;
            }
            if (mux*3 + 2 < NUM_TEMP_CELLS) {
                state->temperature[mux*3 + 2] = ((raw >> 40) & 0xFFFF) * scale + offset;
                state->temp_rx_ms[mux*3 + 2] = frame->rx_ms;
            }
            break;
        }
        case BMU_STATEBUSHV: { // BMU_stateBusHV
            state->voltageBusHV = (raw & 0xFFFF) * 0.01f;
            state->voltageCellMax = ((raw >> 16) & 0xFFFF) * 0.0001f;
            state->voltageCellMin = ((raw >> 32) & 0xFFFF) * 0.0001f;
            state->currentBusHV = ((raw >> 48) & 0xFFFF) * 0.01f;
            state->bus_rx_ms = frame->rx_ms;
            break;
        }
        case BMU_BATTERYSTATUSHV: { // BMU_batteryStatusHV
            state->StateBatteryChargeHV = (raw & 0x03FF) * 0.1f;
            state->StateBatteryHealthHV = ((raw >> 10) & 0x03FF) * 0.1f;
            state->StateBatteryPowerHV = ((raw >> 20) & 0x0FFF) * 0.1f;
            int32_t r = (raw >> 32) & 0x3FF;
            if (r & 0x200) r -= 0x400; // sign extending
            state->TempCellMax = r * 0.25f + 90.0f;
            r = (raw >> 42) & 0x3FF;
            if (r & 0x200) r -= 0x400;
            state->TempCellMin = r * 0.25f + 90.0f;
            state->StateBMS = (raw >> 52) & 0xF;
            state->HighestTempThermistor = (raw >> 56) & 0xFF;
            state->status_rx_ms = frame->rx_ms;
            break;
        }
        case BMU_AMSVBATT: { // BMU_AmsVBatt
            state->AMS_PackVoltage = (raw & 0xFFFF) * 0.01f;
            break;
        }

        default: // traffic from other nodes, ignore
            return 0;
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
    state_t state = {0};
    while (1)  {
        int n = can_read(fd, frames, 32);
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
                int ret = process_frame(&frames[i], &state);
                if (ret < 0) {
                    fprintf(stderr, "Error processing frame #%d (%u)\n", i, frames[i].id);
                }
            }
        }
    }
    can_close(fd);
    return 0;
}