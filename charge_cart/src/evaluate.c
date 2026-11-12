#include <stdint.h>
#include "state.h"
#include "thresholds.h"
#include <float.h>
#include <string.h>
#include "../inc/evaluate.h"

void evaluate_state (const state_t *state, evaluation_t *evaluation, const uint64_t now_ms) {
//update max and min voltages and temps
    float vmax=0, vmin=FLT_MAX, tmax=0, tmin=FLT_MAX;
    int vmax_idx=0, vmin_idx=0, tmax_idx=0, tmin_idx=0;
    double v_sum=0, t_sum=0;
    memset(evaluation->cell_status_counts, 0, sizeof(evaluation->cell_status_counts));
    memset(evaluation->temp_status_counts, 0, sizeof(evaluation->temp_status_counts));
    evaluation->valid_cells = 0;
    evaluation->valid_temps = 0;
    for (int i = 0; i < NUM_VOLTAGE_CELLS; i++) {
        float voltage = state->voltage[i];
        if (state->cell_rx_ms[i] == 0 || voltage < 0.0f || voltage > VOLTAGE_MAX) {
            evaluation->cellStatus[i] = STATUS_INVALID;
            evaluation->cell_status_counts[STATUS_INVALID]++;
            continue;
        }
        else if (now_ms - state->cell_rx_ms[i] > STALE_MS) {
            evaluation->cellStatus[i] = STATUS_STALE;
            evaluation->cell_status_counts[STATUS_STALE]++;
            continue;
        }
        else {
            if (voltage < VOLTAGE_LOW_THRESHOLD) { // fill in threshhods later
                if (voltage < VOLTAGE_LOW_CRITICAL_THRESHOLD) {
                    evaluation->cellStatus[i] = STATUS_LOW_CRITICAL;
                    evaluation->cell_status_counts[STATUS_LOW_CRITICAL]++;
                } else {
                    evaluation->cellStatus[i] = STATUS_LOW;
                    evaluation->cell_status_counts[STATUS_LOW]++;
                }
            } else if (voltage > VOLTAGE_HIGH_THRESHOLD) {
                if (voltage > VOLTAGE_HIGH_CRITICAL_THRESHOLD) {
                    evaluation->cellStatus[i] = STATUS_HIGH_CRITICAL;
                    evaluation->cell_status_counts[STATUS_HIGH_CRITICAL]++;
                } else {
                    evaluation->cellStatus[i] = STATUS_HIGH;
                    evaluation->cell_status_counts[STATUS_HIGH]++;
                }
            } else {
                evaluation->cellStatus[i] = STATUS_OK;
                evaluation->cell_status_counts[STATUS_OK]++;
            }
            if (voltage > vmax) {
                vmax = voltage;
                vmax_idx = i;
            }
            if (voltage < vmin) {
                vmin = voltage;
                vmin_idx = i;
            }
            evaluation->valid_cells++;
            v_sum += voltage;
        }
    }

    for (int i = 0; i < NUM_TEMP_CELLS; i++) {
        float temp = state->temperature[i];
        if (state->temp_rx_ms[i] == 0 || temp < TEMP_MIN || temp > TEMP_MAX) {
            evaluation->tempStatus[i] = STATUS_INVALID;
            evaluation->temp_status_counts[STATUS_INVALID]++;
            continue;
        }
        else if (now_ms - state->temp_rx_ms[i] > STALE_MS) {
            evaluation->tempStatus[i] = STATUS_STALE;
            evaluation->temp_status_counts[STATUS_STALE]++;
            continue;
        }
        else {
            if (temp < TEMP_LOW_THRESHOLD) { // fill in threshhods later
                evaluation->tempStatus[i] = STATUS_LOW;
                evaluation->temp_status_counts[STATUS_LOW]++;
            } else if (temp > TEMP_HIGH_THRESHOLD) {
                evaluation->tempStatus[i] = STATUS_HIGH;
                evaluation->temp_status_counts[STATUS_HIGH]++;
            } else {
                evaluation->tempStatus[i] = STATUS_OK;
                evaluation->temp_status_counts[STATUS_OK]++;
            }
            if (temp > tmax) {
                tmax = temp;
                tmax_idx = i;
            }
            if (temp < tmin) {
                tmin = temp;
                tmin_idx = i;
            }
            evaluation->valid_temps++;
            t_sum += temp;
        }
    }

    double v_avg = evaluation->valid_cells ? v_sum / evaluation->valid_cells : 0.0, t_avg = evaluation->valid_temps ? t_sum / evaluation->valid_temps : 0.0;    
    evaluation->v_max = vmax;
    evaluation->v_min = vmin != FLT_MAX ? vmin : 0.0;
    evaluation->v_avg = v_avg;
    evaluation->v_max_idx = vmax_idx;
    evaluation->v_min_idx = vmin_idx;
    evaluation->t_max = tmax;
    evaluation->t_min = tmin != FLT_MAX ? tmin : 0.0;
    evaluation->t_avg = t_avg;
    evaluation->t_max_idx = tmax_idx;
    evaluation->t_min_idx = tmin_idx;

    evaluation->busStatus = STATUS_OK; // TO DO: implement bus status evaluation logic
    evaluation->AMSStatus = STATUS_OK; // TO DO: implement AMS status evaluation logic
    evaluation->batteryStatus = STATUS_OK; // TO DO: implement battery status evaluation logic

}