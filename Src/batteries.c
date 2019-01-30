#include "batteries.h"

#include "freertos.h"
#include "task.h"
#include "controlStateMachine.h"
#include "debug.h"
#include "math.h"
#include "BMU_can.h"
#include "boardTypes.h"

#define BATTERY_TASK_PERIOD_MS 100

// Cell Low and High Voltages, in volts (floating point)
#define LIMIT_OVERVOLTAGE 4.2F
#define LIMIT_HIGHVOLTAGE 4.2F // TODO: Not sure what this should be
#define LIMIT_LOWVOLTAGE 3.0F // TODO: Not sure what this should be
#define LIMIT_UNDERVOLTAGE 3.0F

#define CELL_TIME_TO_FAILURE_ALLOWABLE (6.0)
#define CELL_DCR (0.01)
#define CELL_HEAT_CAPACITY (1034.2) //kj/kg•k
#define CELL_MASS (0.496)
#define CELL_MAX_TEMP_C (60.0)
#define CELL_OVERTEMP (CELL_MAX_TEMP_C)

QueueHandle_t IBusQueue;
QueueHandle_t VBusQueue;
QueueHandle_t VBattQueue;

/*
 *
 * Platform specific functions
 *
 */

#if IS_BOARD_F7
#include "ltc6811.h"
#include "ade7912.h"
#endif

HAL_StatusTypeDef readBusVoltagesAndCurrents(float *IBus, float *VBus, float *VBatt)
{
#if IS_BOARD_F7
   (*IBus) = adc_read_current();
   (*VBus) = adc_read_v1();
   (*VBatt) = adc_read_v2();
   return HAL_OK;

#elif IS_BOARD_NUCLEO_F7
   // For nucleo, voltages and current can be manually changed via CLI for
   // testing, so we don't do anything here
   return HAL_OK;
#else
#error Unsupported board type
#endif
}

HAL_StatusTypeDef readCellVoltagesAndTemps()
{
#if IS_BOARD_F7
   _Static_assert(VOLTAGECELL_COUNT == NUM_VOLTAGE_CELLS, "Length of array for sending cell voltages over CAN doesn't match number of cells");
   _Static_assert(TEMPCELL_COUNT == NUM_TEMP_CELLS, "Length of array for sending cell temperatures over CAN doesn't match number of temperature cells");

   return batt_read_cell_voltages_and_temps((float *)VoltageCell, (float *)TempCell);
#elif IS_BOARD_NUCLEO_F7
   // For nucleo, cell voltages and temps can be manually changed via CLI for
   // testing, so we don't do anything here
   return HAL_OK;
#else
#error Unsupported board type
#endif
}

/*
 * This functions sets all cell voltages and temps to known values
 * This is necessary for testing on Nucleo so it doesn't immediately error
 * before the user can manually set the Voltages and Temperatures
 */
HAL_StatusTypeDef initVoltageAndTempArrays()
{
#if IS_BOARD_F7
   // For F7 just zero out the array
   float initVoltage = 0;
   float initTemp = 0;
#elif IS_BOARD_NUCLEO_F7
   float initVoltage = LIMIT_OVERVOLTAGE - 0.1;
   float initTemp = CELL_OVERTEMP - 20;
#else
#error Unsupported board type
#endif

   for (int i=0; i<= VOLTAGECELL_COUNT; i++)
   {
      VoltageCell[i] = initVoltage;
   }
   for (int i=0; i<= TEMPCELL_COUNT; i++)
   {
      TempCell[i] = initTemp;
   }

   return HAL_OK;
}

/*
 * This task will retry its readings MAX_ERROR_COUNT times, then fail and send
 * error event
 * TODO: ensure this is max 500 ms
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

HAL_StatusTypeDef initBusVoltagesAndCurrentQueues()
{
   IBusQueue = xQueueCreate(1, sizeof(float));
   VBusQueue = xQueueCreate(1, sizeof(float));
   VBattQueue = xQueueCreate(1, sizeof(float));

   if (IBusQueue == NULL || VBusQueue == NULL || VBattQueue == NULL) {
      ERROR_PRINT("Failed to create bus voltages and current queues!\n");
      return HAL_ERROR;
   }

   return HAL_OK;
}

HAL_StatusTypeDef publishBusVoltagesAndCurrent(float *IBus, float *VBus, float *Vbatt)
{
   xQueueOverwrite(IBusQueue, IBus);
   xQueueOverwrite(VBusQueue, VBus);
   xQueueOverwrite(VBattQueue, Vbatt);

   return HAL_OK;
}

HAL_StatusTypeDef checkCellVoltagesAndTemps(float *maxVoltage, float *minVoltage, float *maxTemp, float *minTemp, float *packVoltage)
{
   HAL_StatusTypeDef rc = HAL_OK;
   float measure;

   *maxVoltage = 0;
   *minVoltage = LIMIT_OVERVOLTAGE;
   *maxTemp = -100; // Cells shouldn't get this cold right??
   *minTemp = CELL_OVERTEMP;
   packVoltage = 0;

   for (int i=0; i < VOLTAGECELL_COUNT; i++)
   {
      measure = VoltageCell[i];

      // Check it is within bounds
      if (measure < LIMIT_UNDERVOLTAGE) {
         ERROR_PRINT("Cell %d is undervoltage at %f Volts\n", i, measure);
         rc = HAL_ERROR;
      }
      if (measure > LIMIT_OVERVOLTAGE) {
         ERROR_PRINT("Cell %d is overvoltage at %f Volts\n", i, measure);
         rc = HAL_ERROR;
      }

      // Update max voltage
      if (measure > (*maxVoltage)) {(*maxVoltage) = measure;}
      if (measure < (*minVoltage)) {(*minVoltage) = measure;}

      // Sum up cell voltages to get overall pack voltage
      (*packVoltage) += measure;
   }

   for (int i=0; i < TEMPCELL_COUNT; i++)
   {
      measure = TempCell[i];

      // Check it is within bounds
      if (measure > CELL_OVERTEMP) {
         ERROR_PRINT("Cell %d is overtemp at %f deg C\n", i, measure);
         rc = HAL_ERROR;
      }

      // Update max voltage
      if (measure > (*maxTemp)) {(*maxTemp) = measure;}
      if (measure < (*minTemp)) {(*minTemp) = measure;}
   }

   return rc;
}


float calculateStateOfPower()
{
   float maxCurrent =  sqrt((((CELL_MAX_TEMP_C - TempCellMax)/CELL_TIME_TO_FAILURE_ALLOWABLE) * (CELL_HEAT_CAPACITY*CELL_MASS))/CELL_DCR);

   return maxCurrent;
}


float calculateStateOfCharge()
{
    return (100000 * (VoltageCellMax - LIMIT_LOWVOLTAGE)) / (LIMIT_HIGHVOLTAGE - LIMIT_LOWVOLTAGE);
}

HAL_StatusTypeDef batteryStart()
{
#if IS_BOARD_F7
   return batt_init();
#elif IS_BOARD_NUCLEO_F7
   // For nucleo, cell voltages and temps can be manually changed via CLI for
   // testing, so we don't do anything here
   return HAL_OK;
#else
#error Unsupported board type
#endif
    return HAL_OK;
}

float VBus;
float VBatt;
float IBus;
void batteryTask(void *pvParameter)
{
    if (initVoltageAndTempArrays() != HAL_OK)
    {
       Error_Handler();
    }

    if (batteryStart() != HAL_OK)
    {
        Error_Handler();
    }

    int errorCounter = 0;
    float packVoltage;
    while (1)
    {
        if (readCellVoltagesAndTemps() != HAL_OK) {
            ERROR_PRINT("Failed to read cell voltages and temperatures!\n");
            BOUNDED_CONTINUE
        }

        if (readBusVoltagesAndCurrents(&IBus, &VBus, &VBatt) != HAL_OK) {
            ERROR_PRINT("Failed to read bus voltages and current!\n");
            BOUNDED_CONTINUE
        }

        if (publishBusVoltagesAndCurrent(&IBus, &VBus, &VBatt) != HAL_OK) {
            ERROR_PRINT("Failed to publish bus voltages and current!\n");
        }

        if (checkCellVoltagesAndTemps(
              ((float *)&VoltageCellMax), ((float *)&VoltageCellMin),
              ((float *)&TempCellMax), ((float *)&TempCellMin),
              &packVoltage) != HAL_OK)
        {
            fsmSendEventUrgent(&fsmHandle, EV_HV_Fault, pdMS_TO_TICKS(500));
            // TODO: What should happen here?
            vTaskSuspend(NULL); // Suspend this task as the system should be shutting down
        }

        StateBatteryPowerHV = calculateStateOfPower();
        StateBatteryChargeHV = calculateStateOfCharge();
        StateBMS = fsmGetState(&fsmHandle);


        /* This sends the following data, all of which get updated each time
         * through the loop
         * - State of Charge
         * - State of Health (not yet implemented)
         * - State of power
         * - TempCellMax
         * - TempCellMin
         * - StateBMS
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
