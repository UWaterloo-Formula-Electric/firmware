#include <stdint.h>
#include "state.h"
#include "thresholds.h"

#define STALE_MS 1000

typedef enum {
    CELL_OK,
    CELL_LOW,
    CELL_HIGH,
    CELL_STALE,
    CELL_INVALID
} cell_status_t;

typedef enum {
    TEMP_OK,
    TEMP_LOW,
    TEMP_HIGH,
    TEMP_STALE,
    TEMP_INVALID
} temp_status_t;

typedef enum {
    BUS_OK,
    BUS_LOW,
    BUS_HIGH,
    BUS_STALE,
    BUS_INVALID
} bus_status_t;

typedef enum {
    AMS_OK,
    AMS_LOW,
    AMS_HIGH,
    AMS_STALE,
    AMS_INVALID
} AMS_status_t;


typedef struct {
    cell_status_t cellStatus[NUM_VOLTAGE_CELLS];
    temp_status_t tempStatus[NUM_TEMP_CELLS];
    bus_status_t busStatus;
    AMS_status_t AMSStatus;
} evaluation_t;

void evaluate_state (const state_t *state, evaluation_t *evaluation, uint64_t now_ms) {

}