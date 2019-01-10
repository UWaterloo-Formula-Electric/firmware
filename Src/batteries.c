#include "batteries.h"

#include "freertos.h"
#include "task.h"
#include "controlStateMachine.h"
#include "debug.h"
#include "math.h"
#include "BMU_can.h"

#define BATTERY_TASK_PERIOD_MS 100

/*
 *
 * Platform specific functions
 *
 */
HAL_StatusTypeDef readCellVoltagesAndTemps()
{
   return HAL_OK;
}

/*
 * This task will retry its readings MAX_ERROR_COUNT times, then fail and send
 * error event
 */
#define MAX_ERROR_COUNT 3
#define BOUNDED_CONTINUE \
    if ((++errorCounter) > MAX_ERROR_COUNT) { \
        fsmSendEventUrgent(&fsmHandle, EV_HV_Fault, pdMS_TO_TICKS(500)); \
    } else { \
        vTaskDelay(pdMS_TO_TICKS(BATTERY_TASK_PERIOD_MS)); \
        continue; \
    } \

// Call on a succesful run through main loop
#define ERROR_COUNTER_SUCCESS() \
    do { \
        errorCounter--; \
    } while (0)


HAL_StatusTypeDef checkCellVoltagesAndTemps()
{
   return HAL_OK;
}

#define CELL_TIME_TO_FAILURE_ALLOWABLE (6.0)
#define CELL_DCR (0.01)
#define CELL_HEAT_CAPACITY (1034.2) //kj/kg•k
#define CELL_MASS (0.496)
#define CELL_MAX_TEMP_C (50.0)

float calculateStateOfPower()
{
   float maxCurrent =  sqrt((((CELL_MAX_TEMP_C - TempCellMax)/CELL_TIME_TO_FAILURE_ALLOWABLE) * (CELL_HEAT_CAPACITY*CELL_MASS))/CELL_DCR);

   return maxCurrent;
}

// Cell Low and High Voltages, in volts (floating point)
// TODO: Update these
#define LIMIT_OVERVOLTAGE 4.25F
#define LIMIT_HIGHVOLTAGE 4.2F
#define LIMIT_LOWVOLTAGE 3.4F
#define LIMIT_UNDERVOLTAGE 3.35F

float calculateStateOfCharge()
{
    return (100000 * (VoltageCellMax - LIMIT_LOWVOLTAGE)) / (LIMIT_HIGHVOLTAGE - LIMIT_LOWVOLTAGE);
}

HAL_StatusTypeDef batteryStart()
{
    return HAL_OK;
}

void batteryTask(void *pvParameter)
{
    if (batteryStart() != HAL_OK)
    {
        Error_Handler();
    }

    int errorCounter = 0;
    while (1)
    {
        if (readCellVoltagesAndTemps() != HAL_OK) {
            ERROR_PRINT("Failed to read cell voltages and temperatures!\n");
            BOUNDED_CONTINUE
        }

        if (checkCellVoltagesAndTemps() != HAL_OK) {
            fsmSendEventUrgent(&fsmHandle, EV_HV_Fault, pdMS_TO_TICKS(500));
            // TODO: What should happen here?
            vTaskSuspend(NULL); // Suspend this task as the system should be shutting down
        }

        StateBatteryPowerHV = calculateStateOfPower();
        StateBatteryChargeHV = calculateStateOfCharge();


        /* This sends the following data, all of which get updated each time
         * through the loop
         * - State of Charge
         * - State of Health (not yet implemented)
         * - State of power
         * - TempCellMax
         * - TempCellMin
         * - StateBMS TODO: Implement
         */
        if (sendCAN_BMU_batteryStatusHV() != HAL_OK) {
            ERROR_PRINT("Failed to send batter status HV\n");
            BOUNDED_CONTINUE
        }

        // Succesfully reach end of loop, update error counter to reflect that
        ERROR_COUNTER_SUCCESS();
        vTaskDelay(pdMS_TO_TICKS(BATTERY_TASK_PERIOD_MS));
    }
}
