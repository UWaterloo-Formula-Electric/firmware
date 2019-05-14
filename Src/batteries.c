#include "batteries.h"

#include "freertos.h"
#include "task.h"
#include "controlStateMachine.h"
#include "debug.h"
#include "math.h"
#include "BMU_can.h"
#include "BMU_dtc.h"
#include "boardTypes.h"
#include "watchdog.h"

#define BATTERY_TASK_PERIOD_MS 100
#define BATTERY_TASK_ID 2

#define HV_MEASURE_TASK_PERIOD_MS 5

#define DISABLE_BATTERY_MONITORING_HARDWARE

// Cell Low and High Voltages, in volts (floating point)
#define LIMIT_OVERVOLTAGE 4.2F
#define LIMIT_HIGHVOLTAGE 4.2F // TODO: Not sure what this should be
#define LIMIT_LOWVOLTAGE 3.0F // TODO: Not sure what this should be
#define LIMIT_UNDERVOLTAGE 3.0F
#define LIMIT_LOWVOLTAGE_WARNING 3.2F

#define CELL_TIME_TO_FAILURE_ALLOWABLE (6.0)
#define CELL_DCR (0.01)
#define CELL_HEAT_CAPACITY (1034.2) //kj/kg•k
#define CELL_MASS (0.496)
#define CELL_MAX_TEMP_C (60.0)
#define CELL_OVERTEMP (CELL_MAX_TEMP_C)
#define CELL_OVERTEMP_WARNING (CELL_MAX_TEMP_C - 10)

#define BALANCE_START_VOLTAGE (3.5F)
#define CELL_RELAXATION_TIME_MS (10)
#define CHARGE_STOP_VOLTAGE (LIMIT_OVERVOLTAGE-0.03)
#define CHARGE_TIMEOUT_MS (600000)

// This should be long enough so cells aren't constantly being toggled
// between balance and not
#define BALANCE_RECHECK_PERIOD_MS (3000)

QueueHandle_t IBusQueue;
QueueHandle_t VBusQueue;
QueueHandle_t VBattQueue;

// Declared global for testing
float VBus;
float VBatt;
float IBus;
float packVoltage;


bool warningSentForCellVoltage[VOLTAGECELL_COUNT];
bool warningSentForCellTemp[TEMPCELL_COUNT];

#define NUM_SOC_LOOKUP_VALS 101
float voltageToSOCLookup[NUM_SOC_LOOKUP_VALS] = {
   0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
   21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
   41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60,
   61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80,
   81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100
};

/*
 *
 * Platform specific functions
 *
 */

#if IS_BOARD_F7
#include "ltc6811.h"
#include "ade7912.h"
#include "imdDriver.h"
#endif

HAL_StatusTypeDef readBusVoltagesAndCurrents(float *IBus, float *VBus, float *VBatt)
{
#if IS_BOARD_F7 && !defined(DISABLE_BATTERY_MONITORING_HARDWARE)
   (*IBus) = adc_read_current();
   (*VBus) = adc_read_v1();
   (*VBatt) = adc_read_v2();
   return HAL_OK;

#elif IS_BOARD_NUCLEO_F7 || defined(DISABLE_BATTERY_MONITORING_HARDWARE)
   // For nucleo, voltages and current can be manually changed via CLI for
   // testing, so we don't do anything here
   return HAL_OK;
#else
#error Unsupported board type
#endif
}

HAL_StatusTypeDef readCellVoltagesAndTemps()
{
#if IS_BOARD_F7 && !defined(DISABLE_BATTERY_MONITORING_HARDWARE)
   _Static_assert(VOLTAGECELL_COUNT == NUM_VOLTAGE_CELLS, "Length of array for sending cell voltages over CAN doesn't match number of cells");
   _Static_assert(TEMPCELL_COUNT == NUM_TEMP_CELLS, "Length of array for sending cell temperatures over CAN doesn't match number of temperature cells");

   return batt_read_cell_voltages_and_temps((float *)VoltageCell, (float *)TempCell);
#elif IS_BOARD_NUCLEO_F7 || defined(DISABLE_BATTERY_MONITORING_HARDWARE)
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
#if IS_BOARD_F7 && !defined(DISABLE_BATTERY_MONITORING_HARDWARE)
   // For F7 just zero out the array
   float initVoltage = 0;
   float initTemp = 0;
#elif IS_BOARD_NUCLEO_F7 || defined(DISABLE_BATTERY_MONITORING_HARDWARE)
   float initVoltage = LIMIT_OVERVOLTAGE - 0.1;
   float initTemp = CELL_OVERTEMP - 20;
#else
#error Unsupported board type
#endif

   for (int i=0; i<= VOLTAGECELL_COUNT; i++)
   {
      VoltageCell[i] = initVoltage;
      warningSentForCellVoltage[i] = false;
   }
   for (int i=0; i<= TEMPCELL_COUNT; i++)
   {
      TempCell[i] = initTemp;
      warningSentForCellTemp[i] = false;
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
        watchdogTaskCheckIn(BATTERY_TASK_ID); \
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

HAL_StatusTypeDef publishBusVoltagesAndCurrent(float *pIBus, float *pVBus, float *pVBatt)
{
   xQueueOverwrite(IBusQueue, pIBus);
   xQueueOverwrite(VBusQueue, pVBus);
   xQueueOverwrite(VBattQueue, pVBatt);

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
   *packVoltage = 0;

   for (int i=0; i < VOLTAGECELL_COUNT; i++)
   {
      measure = VoltageCell[i];

      // Check it is within bounds
      if (measure < LIMIT_UNDERVOLTAGE) {
         ERROR_PRINT("Cell %d is undervoltage at %f Volts\n", i, measure);
         sendDTC_CRITICAL_CELL_VOLTAGE_LOW(i);
         rc = HAL_ERROR;
      } else if (measure > LIMIT_OVERVOLTAGE) {
         ERROR_PRINT("Cell %d is overvoltage at %f Volts\n", i, measure);
         sendDTC_CRITICAL_CELL_VOLTAGE_HIGH(i);
         rc = HAL_ERROR;
      } else if (measure < LIMIT_LOWVOLTAGE_WARNING) {
         if (!warningSentForCellVoltage[i]) {
            ERROR_PRINT("WARN: Cell %d is low voltage at %f Volts\n", i, measure);
            sendDTC_WARNING_CELL_VOLTAGE_LOW(i);
            warningSentForCellVoltage[i] = true;
         }
      } else if (warningSentForCellVoltage[i] == true) {
         warningSentForCellVoltage[i] = false;
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
         sendDTC_CRITICAL_CELL_TEMP_HIGH(i);
         rc = HAL_ERROR;
      } else if (measure > CELL_OVERTEMP_WARNING) {
         if (!warningSentForCellTemp[i]) {
            ERROR_PRINT("WARN: Cell %d is high temp at %f deg C\n", i, measure);
            sendDTC_WARNING_CELL_TEMP_HIGH(i);
            warningSentForCellTemp[i] = true;
         }
      } else if (warningSentForCellTemp[i] == true) {
         warningSentForCellTemp[i] = false;
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
#if IS_BOARD_F7 && !defined(DISABLE_BATTERY_MONITORING_HARDWARE)
   return batt_init();
#elif IS_BOARD_NUCLEO_F7 || defined(DISABLE_BATTERY_MONITORING_HARDWARE)
   // For nucleo, cell voltages and temps can be manually changed via CLI for
   // testing, so we don't do anything here
   return HAL_OK;
#else
#error Unsupported board type
#endif
    return HAL_OK;
}

void HVMeasureTask(void *pvParamaters)
{
    if (hvadc_init() != HAL_OK)
    {
       ERROR_PRINT("Failed to init HV ADC\n");
    }

    while (1) {
        if (readBusVoltagesAndCurrents(&IBus, &VBus, &VBatt) != HAL_OK) {
            ERROR_PRINT("Failed to read bus voltages and current!\n");
        }

        if (publishBusVoltagesAndCurrent(&IBus, &VBus, &VBatt) != HAL_OK) {
            ERROR_PRINT("Failed to publish bus voltages and current!\n");
        }
        vTaskDelay(HV_MEASURE_TASK_PERIOD_MS);
    }
}

HAL_StatusTypeDef startCharge()
{
   DEBUG_PRINT("Starting charge\n");
   return HAL_OK;
}

HAL_StatusTypeDef stopCharge()
{
   DEBUG_PRINT("stopping charge\n");
   return HAL_OK;
}

HAL_StatusTypeDef stopBalance()
{
   if (batt_unset_balancing_all_cells() != HAL_OK) {
      return HAL_ERROR;
   }

   if (batt_write_config() != HAL_OK) {
      return HAL_ERROR;
   }

   return HAL_OK;
}

bool isCellBalancing[VOLTAGECELL_COUNT] = {0};
HAL_StatusTypeDef pauseBalance()
{
   for (int cell = 0; cell < VOLTAGECELL_COUNT; cell++) {
      if (batt_is_cell_balancing(cell)) {
         isCellBalancing[cell] = true;
      } else {
         isCellBalancing[cell] = false;
      }
   }

   if (stopBalance() != HAL_OK) {
      ERROR_PRINT("Failed to pause balance\n");
   }

   return HAL_OK;
}

HAL_StatusTypeDef resumeBalance()
{
   for (int cell = 0; cell < VOLTAGECELL_COUNT; cell++) {
      if (isCellBalancing[cell]) {
         batt_balance_cell(cell);
      }
   }

   if (batt_write_config() != HAL_OK) {
      ERROR_PRINT("Failed to resume balance\n");
   }

   return HAL_OK;
}


float map_range_float(float in, float low, float high, float low_out, float high_out) {
    if (in < low) {
        in = low;
    } else if (in > high) {
        in = high;
    }
    float in_range = high - low;
    float out_range = high_out - low_out;

    return (in - low) * out_range / in_range + low_out;
}

float getSOCFromVoltage(float cellVoltage)
{
   // mV per step 11.88
   float VoltsPerLookup = (LIMIT_OVERVOLTAGE - LIMIT_UNDERVOLTAGE) / (NUM_SOC_LOOKUP_VALS-1);
   float lookupIndex = (cellVoltage - LIMIT_UNDERVOLTAGE) / VoltsPerLookup;
   if (lookupIndex < 0) { lookupIndex = 0;}
   if (lookupIndex > (NUM_SOC_LOOKUP_VALS-1)) { lookupIndex = (NUM_SOC_LOOKUP_VALS-1);}
   int lookupIndexInt = (int)lookupIndex;

   DEBUG_PRINT("lookupIndex %f, int %d\n", lookupIndex, lookupIndexInt);

   // linear interpolation
   float soc;
   if (lookupIndex == 0) {
      soc = voltageToSOCLookup[0];
   } else if (lookupIndexInt >= (NUM_SOC_LOOKUP_VALS-1)) {
      soc = voltageToSOCLookup[NUM_SOC_LOOKUP_VALS-1];
   } else {
      soc = map_range_float(lookupIndex, lookupIndexInt, lookupIndexInt+1,
                            voltageToSOCLookup[lookupIndexInt],
                            voltageToSOCLookup[lookupIndexInt+1]);
   }
   return soc;
}

HAL_StatusTypeDef balanceCharge()
{
   uint32_t errorCounter = 0;

   // Start charge
   if (startCharge() != HAL_OK) {
      return HAL_ERROR;
   }

   uint32_t startTickCount = xTaskGetTickCount();
   bool balancingCells = false; // Are we balancing any cell currently?
   uint32_t lastBalanceCheck = 0;

   while (1) {
      /*
       * Check for timeout
       */
      if (xTaskGetTickCount() - startTickCount > pdMS_TO_TICKS(CHARGE_TIMEOUT_MS))
      {
         ERROR_PRINT("Timed out waiting for charge to finish\n");

         stopCharge();
         batt_unset_balancing_all_cells();
         batt_write_config();
         return HAL_ERROR;
      }

      /*
       * Perform cell reading, need to pause any ongoing balance in order to
       * get good voltage readings
       * After we have read, we can re-enable balancing on cells
       */
      balancingCells = false;
      if (pauseBalance() != HAL_OK) {
         ERROR_PRINT("Failed to pause balance!\n");
         BOUNDED_CONTINUE
      }
      vTaskDelay(pdMS_TO_TICKS(CELL_RELAXATION_TIME_MS));

      if (readCellVoltagesAndTemps() != HAL_OK) {
         ERROR_PRINT("Failed to read cell voltages and temperatures!\n");
         BOUNDED_CONTINUE
      }

      if (resumeBalance() != HAL_OK) {
         ERROR_PRINT("Failed to pause balance!\n");
         BOUNDED_CONTINUE
      }


      /*
       * Safety checks for cells
       */
      if (checkCellVoltagesAndTemps(
            ((float *)&VoltageCellMax), ((float *)&VoltageCellMin),
            ((float *)&TempCellMax), ((float *)&TempCellMin),
            &packVoltage) != HAL_OK)
      {
         fsmSendEventUrgent(&fsmHandle, EV_HV_Fault, pdMS_TO_TICKS(500));
         while (1) {
            // Suspend this task while still updating watchdog
            watchdogTaskCheckIn(BATTERY_TASK_ID);
            vTaskDelay(pdMS_TO_TICKS(BATTERY_TASK_PERIOD_MS));
         }
      }

      // Only balance above a minimum voltage
      if (VoltageCellMin >= BALANCE_START_VOLTAGE)
      {
         if (xTaskGetTickCount() - lastBalanceCheck
             > pdMS_TO_TICKS(BALANCE_RECHECK_PERIOD_MS))
         {
            DEBUG_PRINT("Starting balance\n");
            DEBUG_PRINT("Voltages:\n");
            for (int cell = 0; cell < NUM_VOLTAGE_CELLS; cell++) {
               DEBUG_PRINT("%d: %f,", cell, VoltageCell[cell]);
            }
            DEBUG_PRINT("\n");
            DEBUG_PRINT("Voltage min %f, max %f\n\n", VoltageCellMin, VoltageCellMax);
            float minCellSOC = getSOCFromVoltage(VoltageCellMin);

            for (int cell=0; cell<VOLTAGECELL_COUNT; cell++) {
               float cellSOC = getSOCFromVoltage(VoltageCell[cell]);
               DEBUG_PRINT("Cell %d SOC: %f\n", cell, cellSOC);

               if (cellSOC - minCellSOC > 1) {
                  DEBUG_PRINT("Balancing cell %d\n", cell);
                  batt_balance_cell(cell);
                  balancingCells = true;
               }
            }

            DEBUG_PRINT("\n\n\n");

            /*batt_set_disharge_timer(DT_30_SEC);*/
            /*if (batt_write_config() != HAL_OK)*/
            /*{*/
            /*return HAL_ERROR;*/
            /*}*/

            lastBalanceCheck = xTaskGetTickCount();
         }
      } else {
         if (stopBalance() != HAL_OK) {
            ERROR_PRINT("Failed to stop balance\n");
            BOUNDED_CONTINUE
         }
      }


      if (VoltageCellMin >= CHARGE_STOP_VOLTAGE && !balancingCells) {
         DEBUG_PRINT("Done charging\n");
         if (stopCharge() != HAL_OK) {
            batt_unset_balancing_all_cells();
            /*batt_write_config();*/
            return HAL_ERROR;
         }
         break;
      }

      // Succesfully reach end of loop, update error counter to reflect that
      ERROR_COUNTER_SUCCESS();
      /*!!! Change the check in in bounded continue as well if you change
       * this */
      /*watchdogTaskCheckIn(BATTERY_TASK_ID);*/
      vTaskDelay(pdMS_TO_TICKS(BATTERY_TASK_PERIOD_MS));
   }

   return HAL_OK;
}

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

#if IS_BOARD_F7 && !defined(DISABLE_BATTERY_MONITORING_HARDWARE)
    // This runs in the background via interrupts
    if (begin_imd_measurement() != HAL_OK)
    {
        Error_Handler();
    }
#endif

    if (registerTaskToWatch(BATTERY_TASK_ID, 2*pdMS_TO_TICKS(BATTERY_TASK_PERIOD_MS), false, NULL) != HAL_OK)
    {
        ERROR_PRINT("Failed to register battery task with watchdog!\n");
        Error_Handler();
    }

    int errorCounter = 0;
    while (1)
    {
        if (checkForOpenCircuit() != HAL_OK) {
            ERROR_PRINT("Open wire test failed!\n");
            fsmSendEventUrgent(&fsmHandle, EV_HV_Fault, pdMS_TO_TICKS(500));
            while (1) {
               // Suspend this task while still updating watchdog
               watchdogTaskCheckIn(BATTERY_TASK_ID);
               vTaskDelay(pdMS_TO_TICKS(BATTERY_TASK_PERIOD_MS));
            }
        }
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
            while (1) {
               // Suspend this task while still updating watchdog
               watchdogTaskCheckIn(BATTERY_TASK_ID);
               vTaskDelay(pdMS_TO_TICKS(BATTERY_TASK_PERIOD_MS));
            }
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
        /*!!! Change the check in in bounded continue as well if you change
         * this */
        watchdogTaskCheckIn(BATTERY_TASK_ID);
        vTaskDelay(pdMS_TO_TICKS(BATTERY_TASK_PERIOD_MS));
    }
}
