#include <stdint.h>
#include "state.h"
#include "thresholds.h"
#include <string.h>
#include <float.h>

#define STALE_MS 1000

typedef enum {
    STATUS_OK,
    STATUS_LOW,
    STATUS_HIGH,
    STATUS_STALE,
    STATUS_INVALID,
    STATUS_LOW_CRITICAL,
    STATUS_HIGH_CRITICAL
} status_t;

typedef struct {
    status_t cellStatus[NUM_VOLTAGE_CELLS];
    status_t tempStatus[NUM_TEMP_CELLS];

    float v_max, v_min, v_avg, t_max, t_min, t_avg;
    int v_max_idx, v_min_idx, t_max_idx, t_min_idx, valid_cells, valid_temps;

    int cell_status_counts[5];
    int temp_status_counts[5];
    status_t busStatus;
    status_t AMSStatus;
    status_t batteryStatus;
} evaluation_t;

void evaluate_state (const state_t *state, evaluation_t *evaluation, const uint64_t now_ms);