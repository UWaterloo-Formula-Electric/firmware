/**
  *****************************************************************************
  * @file    batteries.c
  * @author  Richard Matthews
  * @brief   Battery Monitoring, Charging, HV Bus Sense, IMD monitor
  * @details This file contains a number of tasks and related functions:
  *          - Battery Monitoring and Charging (batteryTask): Monitors the
  *          voltage and temperature of all cells by communicating with the AMS
  *          boards, performs balance charging and communicates with the
  *          charger over CAN
  *          - HV Bus Sense (HVMeasureTask): Measures the voltage and current
  *          on the HV Bus as well as the HV battery pack voltage
  *          - IMD Monitoring (imdTask): Measures the signals from the
  *          insulation monitoring device (IMD) to check for insulation faults
  *          - canSendCellTask: Sends the cell voltage and temperatures over
  *          CAN using the multiplexed CAN messages
  *
  *****************************************************************************
  */
#include "batteries.h"

#include "FreeRTOS.h"
#include "cmsis_os.h"
#include "task.h"
#include "controlStateMachine.h"
#include "debug.h"
#include "math.h"
#include "bmu_can.h"
#include "bmu_dtc.h"
#include "boardTypes.h"
#include "watchdog.h"
#include "canReceive.h"
#include "chargerControl.h"
#include "state_of_charge.h"

/*
 *
 * Platform specific includes
 *
 */
#if IS_BOARD_F7
#include "ltc_chip.h"
#include "ltc_chip_interface.h"
#include "ade7912.h"
#include "imdDriver.h"
#endif


/*
 * Defines to enable/disable different functionality for testing purposes
 */

#define ENABLE_IMD
#define ENABLE_HV_MEASURE
#define ENABLE_AMS
#define ENABLE_CHARGER
#define ENABLE_BALANCE


extern osThreadId BatteryTaskHandle;

/// Charging current limit
float maxChargeCurrent = CHARGE_DEFAULT_MAX_CURRENT;

float adjustedCellIR = ADJUSTED_CELL_IR_DEFAULT;

/**
 * Charging voltage limit to be sent to charger. Charging is actually stopped based on min cell SoC as specified by @ref CHARGE_STOP_SOC
 */
float maxChargeVoltage = DEFAULT_LIMIT_OVERVOLTAGE * NUM_VOLTAGE_CELLS;

// Limits for Under/Over Voltage - Can be overwritten from the CLI
volatile float limit_overvoltage = DEFAULT_LIMIT_OVERVOLTAGE;
volatile float limit_undervoltage = DEFAULT_LIMIT_UNDERVOLTAGE;


/**
 * Set to true if we've already sent a high temperature warning for this cell to
 * stop repeated warnings being sent. Reset to false on init or when cell
 * voltage increases above high temperature warning limit
 */
bool warningSentForChannelTemp[NUM_TEMP_CELLS];

#define NUM_SOC_LOOKUP_VALS 101

/**
 * Lookup table to convert cell voltage to cell state of charge.
 * Values between points are linearly interpolated.
 *
 * Currently just a linear mapping. Substitute for a suitable non-linear
 * relationship when it has been developed.
 */
float voltageToSOCLookup[NUM_SOC_LOOKUP_VALS] = {
   0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
   21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
   41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60,
   61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80,
   81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100
};

/*
 * HV Measure task Defines and Variables
 */
#define HV_MEASURE_TASK_PERIOD_MS 1
#define HV_MEASURE_TASK_ID 4
#define STATE_BUS_HV_CAN_SEND_PERIOD_MS 100
static uint32_t StateBusHVSendPeriod = STATE_BUS_HV_CAN_SEND_PERIOD_MS;

/// Queue holding most recent bus current measurement
QueueHandle_t IBusQueue;
/// Queue holding most recent bus volage measurement
QueueHandle_t VBusQueue;
/// Queue holding most recent battery voltage measurement
QueueHandle_t VBattQueue;
/// Queue holding most recent battery  voltage (calculated from sum of cell voltages) measurement
QueueHandle_t PackVoltageQueue;
/// Queue holding most recent adjusted pack voltage (calculated from sum of filtered and adjusted cell voltages) measurement
QueueHandle_t AdjustedPackVoltageQueue;


/*
 * IMD Task defines and variables
 */
#define IMD_TASK_PERIOD_MS 1000
#define IMD_TASK_ID 5

/// Array is used to store the filtered voltages
float cellVoltagesFiltered[NUM_VOLTAGE_CELLS];

extern osThreadId stateOfChargeHandle;

/*
 * HV Measure
 */

/**
 * @brief Filter constant for HV Bus current low pass filter
 * Designed around 1 ms sample period and 50 Hz cuttoff.
 * See the wikipedia page for the calculation: https://en.wikipedia.org/wiki/Low-pass_filter#Simple_infinite_impulse_response_filter
 */
#define IBUS_FILTER_ALPHA 0.24


// Filter constant for Filtered Cell Voltages
// Designed for 75 ms sample period and 1 Hz cutoff
#define CELL_FILTER_ALPHA 0.05 

/**
 * @brief Low pass filters the HV Bus current measurement
 * @param[in] IBus the measured HV Bus current
 * @return The filtered HV Bus current
 */
float filterIBus(float IBus)
{
  static float IBusOut = 0;

  IBusOut = IBUS_FILTER_ALPHA*IBus + (1-IBUS_FILTER_ALPHA)*IBusOut;

  return IBusOut;
}

/**
 * @brief Creates queues for HV Bus measurements
 * Queue overwrite, Queue peek, and queue size of 1 is used to ensure only the most recent
 * data is in the queue and read from the queue. Queues use Amps and Volts for
 * units
 */
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

/**
 * @brief Publishes the most recent HV Bus measurements to queues for other
 * tasks to read from Queue overwrite, Queue peek, and queue size of 1 is used
 * to ensure only the most recent data is in the queue and read from the queue
 *
 * @param[in] pIBus pointer to the HV bus current measurement (in Amps)
 * @param[in] pVBus pointer to the HV bus voltage measurement (in Volts)
 * @param[in] pVBatt pointer to the HV battery voltage measurement (in Volts)
 *
 * @return HAL_StatusTypeDef
 */
HAL_StatusTypeDef publishBusVoltagesAndCurrent(float *pIBus, float *pVBus, float *pVBatt)
{
   xQueueOverwrite(IBusQueue, pIBus);
   xQueueOverwrite(VBusQueue, pVBus);
   xQueueOverwrite(VBattQueue, pVBatt);

   return HAL_OK;
}

/**
 * @brief Measures HV voltages and currents using the HV ADC
 *
 * @param[out] IBus pointer to the HV bus current measurement (in Amps)
 * @param[out] VBus pointer to the HV bus voltage measurement (in Volts)
 * @param[out] VBatt pointer to the HV battery voltage measurement (in Volts)
 *
 * @return HAL_StatusTypeDef
 */
HAL_StatusTypeDef readBusVoltagesAndCurrents(float *IBus, float *VBus, float *VBatt)
{
#if IS_BOARD_F7 && defined(ENABLE_HV_MEASURE)
   float IBusTmp = 0;
   if (adc_read_current(&IBusTmp) != HAL_OK) {
      ERROR_PRINT("Error reading IBUS\n");
      return HAL_ERROR;
   }

   (*IBus) = filterIBus(IBusTmp);

   if (adc_read_v1(VBus) != HAL_OK) {
      ERROR_PRINT("Error reading VBUS\n");
      return HAL_ERROR;
   }
   if (adc_read_v2(VBatt) != HAL_OK) {
      ERROR_PRINT("Error reading VBatt\n");
      return HAL_ERROR;
   }
   return HAL_OK;

#elif IS_BOARD_NUCLEO_F7 || !defined(ENABLE_HV_MEASURE)
   // For nucleo, voltages and current can be manually changed via CLI for
   // testing, so we don't do anything here
   return HAL_OK;
#else
#error Unsupported board type
#endif
}

/**
 * @brief Get the most recent bus current reading
 *
 * @param[out] IBus pointer to a float to store the IBus reading, in amps
 *
 * @return HAL_StatusTypeDef
 */
HAL_StatusTypeDef getIBus(float *IBus)
{
    if (xQueuePeek(IBusQueue, IBus, 0) != pdTRUE) {
        ERROR_PRINT("Failed to receive IBus current from queue\n");
        return HAL_ERROR;
    }

    return HAL_OK;
}

/**
 * @brief Get the most recent battery voltage reading (from the HV ADC). Note:
 * this is only equal to the battery voltage when both contactors are closed.
 * to get the battery voltage in all times, use @ref getPackVoltage
 *
 * @param[out] VBatt pointer to a float to store the VBatt reading, in volts
 *
 * @return HAL_StatusTypeDef
 */
HAL_StatusTypeDef getVBatt(float *VBatt)
{
    if (xQueuePeek(VBattQueue, VBatt, 0) != pdTRUE) {
        ERROR_PRINT("Failed to receive VBatt voltage from queue\n");
        return HAL_ERROR;
    }

    return HAL_OK;
}

/**
 * @brief Get the most recent HV Bus voltage reading (from the HV ADC)
 *
 * @param[out] VBus: pointer to a float to store the VBus reading, in volts
 *
 * @return HAL_StatusTypeDef
 */
HAL_StatusTypeDef getVBus(float * VBus)
{
    if (xQueuePeek(VBusQueue, VBus, 0) != pdTRUE) {
        ERROR_PRINT("Failed to receive VBus voltage from queue\n");
        return HAL_ERROR;
    }

    return HAL_OK;
}

/**
 * @brief For nucleo testing, allows setting a dummy value for the VBatt
 * reading
 *
 * @param VBatt The value to set for VBatt, in volts
 *
 * @return HAL_StatusTypeDef
 */
HAL_StatusTypeDef cliSetVBatt(float VBatt)
{
   xQueueOverwrite(VBattQueue, &VBatt);

   return HAL_OK;
}

/**
 * @brief For nucleo testing, allows setting a dummy value for the VBus
 * reading
 *
 * @param VBus The value to set for VBus, in volts
 *
 * @return HAL_StatusTypeDef
 */
HAL_StatusTypeDef cliSetVBus(float VBus)
{
   xQueueOverwrite(VBusQueue, &VBus);

   return HAL_OK;
}

/**
 * @brief For nucleo testing, allows setting a dummy value for the IBus
 * reading
 *
 * @param IBus The value to set for IBus, in volts
 *
 * @return HAL_StatusTypeDef
 */
HAL_StatusTypeDef cliSetIBus(float IBus)
{
   xQueueOverwrite(IBusQueue, &IBus);

   return HAL_OK;
}

/**
 * @brief Allows setting of the interval for sending state bus HV CAN messages
 *
 * @param period The period of time, in milliseconds
 */
void cliSetStateBusHVSendPeriod(uint32_t period)
{
    StateBusHVSendPeriod = period;
}

/**
 * @brief Allows getting of the interval for sending state bus HV CAN messages
 */
uint32_t cliGetStateBusHVSendPeriod() 
{
    return StateBusHVSendPeriod;
}


/**
 * Measures the voltage and current on the HV Bus as well as the HV battery
 * pack voltage.
 *
 * Note that the HV battery pack voltage (VBatt) is only equal to the actual
 * pack voltage in certain contactor states. See documentation on Precharge
 * / Discharge for more information. Instead, use @ref getPackVoltage which
 * bases it measurement on the sum of cell voltages
 */
void HVMeasureTask(void *pvParamaters)
{
#if IS_BOARD_F7
    if (hvadc_init() != HAL_OK)
    {
       ERROR_PRINT("Failed to init HV ADC\n");
    }
#endif

    if (registerTaskToWatch(HV_MEASURE_TASK_ID, 5*pdMS_TO_TICKS(HV_MEASURE_TASK_PERIOD_MS), false, NULL) != HAL_OK)
    {
        ERROR_PRINT("Failed to register hv measure task with watchdog!\n");
        Error_Handler();
    }

    uint32_t lastStateBusHVSend = 0;

    float VBus;
    float VBatt;
    float IBus;
    TickType_t xLastWakeTime = xTaskGetTickCount();
    while (1) {
        if (readBusVoltagesAndCurrents(&IBus, &VBus, &VBatt) != HAL_OK) {
            ERROR_PRINT("Failed to read bus voltages and current!\n");
        }

        if (publishBusVoltagesAndCurrent(&IBus, &VBus, &VBatt) != HAL_OK) {
            ERROR_PRINT("Failed to publish bus voltages and current!\n");
        }


        if (xTaskGetTickCount() - lastStateBusHVSend
            > pdMS_TO_TICKS(StateBusHVSendPeriod))
        {
            CurrentBusHV = IBus;
            VoltageBusHV = VBus;
            sendCAN_BMU_stateBusHV();
            vTaskDelay(2); // Added to prevent CAN mailbox full
            AMS_PackVoltage = VBatt;
            sendCAN_BMU_AmsVBatt();
            lastStateBusHVSend = xTaskGetTickCount();
        }
        integrate_bus_current(IBus, (float)HV_MEASURE_TASK_PERIOD_MS);
    
        watchdogTaskCheckIn(HV_MEASURE_TASK_ID);
        vTaskDelayUntil(&xLastWakeTime, HV_MEASURE_TASK_PERIOD_MS);
    }
}

/*
 * IMD
 */

/**
 * @brief Monitors the Insulation Monitoring Device (IMD). Raises error if IMD
 * reports fault
 */
void imdTask(void *pvParamaters)
{
#if IS_BOARD_F7 && defined(ENABLE_IMD)
   IMDStatus imdStatus;

   if (begin_imd_measurement() != HAL_OK) {
      ERROR_PRINT("Failed to start IMD measurement\n");
      Error_Handler();
   }

   // Wait for IMD to startup
   DEBUG_PRINT("Waiting for IMD...");
   do {
      imdStatus = get_imd_status();
      vTaskDelay(100);
   } while (!(imdStatus == IMDSTATUS_Normal || imdStatus == IMDSTATUS_SST_Good));

   // Notify control fsm that IMD is ready
   fsmSendEvent(&fsmHandle, EV_IMD_Ready, portMAX_DELAY);

   if (registerTaskToWatch(IMD_TASK_ID, 2*pdMS_TO_TICKS(IMD_TASK_PERIOD_MS), false, NULL) != HAL_OK)
   {
     ERROR_PRINT("Failed to register imd task with watchdog!\n");
     Error_Handler();
   }
   TickType_t xLastWakeTime = xTaskGetTickCount();
   while (1) {
      imdStatus =  get_imd_status();

      switch (imdStatus) {
         case IMDSTATUS_Normal:
         case IMDSTATUS_SST_Good:
            // All good
            break;
         case IMDSTATUS_Invalid:
            ERROR_PRINT_ISR("Invalid IMD measurement\n");
            break;
         case IMDSTATUS_Undervoltage:
            ERROR_PRINT_ISR("IMD Status: Undervoltage\n");
            sendDTC_FATAL_IMD_Failure(imdStatus);
            break;
         case IMDSTATUS_SST_Bad:
            ERROR_PRINT_ISR("IMD Status: SST_Bad\n");
            sendDTC_FATAL_IMD_Failure(imdStatus);
            break;
         case IMDSTATUS_Device_Error:
            ERROR_PRINT_ISR("IMD Status: Device Error\n");
            sendDTC_FATAL_IMD_Failure(imdStatus);
            break;
         case IMDSTATUS_Fault_Earth:
            ERROR_PRINT_ISR("IMD Status: Fault Earth\n");
            sendDTC_FATAL_IMD_Failure(imdStatus);
            break;
         case IMDSTATUS_HV_Short:
            ERROR_PRINT_ISR("IMD Status: fault hv short\n");
            sendDTC_FATAL_IMD_Failure(imdStatus);
            break;
         default:
            ERROR_PRINT_ISR("Unkown IMD Status\n");
            sendDTC_FATAL_IMD_Failure(imdStatus);
            break;
      }

      if (!(imdStatus == IMDSTATUS_Normal || imdStatus == IMDSTATUS_SST_Good))
      {
         // ERROR!!!
         fsmSendEventUrgentISR(&fsmHandle, EV_HV_Fault);
      }

      watchdogTaskCheckIn(IMD_TASK_ID);
      vTaskDelayUntil(&xLastWakeTime, IMD_TASK_PERIOD_MS);
   }
#else
   // Notify control fsm that IMD is ready
   fsmSendEvent(&fsmHandle, EV_IMD_Ready, portMAX_DELAY);
   while (1) {
      vTaskDelay(IMD_TASK_PERIOD_MS);
   }
#endif
}


/*
 * Battery cell Monitoring and Charging
 */


/**
 * @brief Reads the cell voltages and temperatures from the AMS boards. The
 * battery temperature and cell voltages are stored in the global arrays which
 * are also used for sending them over CAN.
 *
 * @return HAL_StatusTypeDef
 */
HAL_StatusTypeDef readCellVoltagesAndTemps()
{
#if IS_BOARD_F7 && defined(ENABLE_AMS)
   /*_Static_assert(VOLTAGECELL_COUNT == NUM_VOLTAGE_CELLS, "Length of array for sending cell voltages over CAN doesn't match number of cells");*/
   /*_Static_assert(TEMPCELL_COUNT == NUM_TEMP_CELLS, "Length of array for sending cell temperatures over CAN doesn't match number of temperature cells");*/
   return batt_read_cell_voltages_and_temps((float *)VoltageCell, (float *)TempChannel);
#elif IS_BOARD_NUCLEO_F7 || !defined(ENABLE_AMS)
   // For nucleo, cell voltages and temps can be manually changed via CLI for
   // testing, so we don't do anything here
   return HAL_OK;
#else
#error Unsupported board type
#endif
}

/*
 * Adjusts cell voltages based on cell internal resistance and current
 * */
void enterAdjustedCellVoltages(void)
{
    static bool filter = false;
    float bus_current_A;
    getIBus(&bus_current_A);
    for (int cell = 0; cell < NUM_VOLTAGE_CELLS; cell++)
    {
        float adjusted_cell_v = VoltageCell[cell] + (bus_current_A * adjustedCellIR);
        if(filter)
        {
            AdjustedVoltageCell[cell] = CELL_FILTER_ALPHA*adjusted_cell_v + (1-CELL_FILTER_ALPHA)*AdjustedVoltageCell[cell];
        }
        else
        {
            AdjustedVoltageCell[cell] = adjusted_cell_v;
        }
    }
    filter = true;
}
/**
 * @brief This functions sets all cell voltages and temps to known values.
 * This is necessary for testing on Nucleo so it doesn't immediately error
 * before the user can manually set the Voltages and Temperatures
 */
HAL_StatusTypeDef initVoltageAndTempArrays()
{
#if IS_BOARD_F7 && defined(ENABLE_AMS)
   // For F7 just zero out the array
   float initVoltage = 3.5;
   float initTemp = 0;
#elif IS_BOARD_NUCLEO_F7 || !defined(ENABLE_AMS)
   float initVoltage = limit_overvoltage - 0.1;
   float initTemp = CELL_OVERTEMP - 20;
#else
#error Unsupported board type
#endif

   for (int i=0; i < NUM_VOLTAGE_CELLS; i++)
   {
      VoltageCell[i] = initVoltage;
   }
   for (int i=0; i < NUM_TEMP_CELLS; i++)
   {
      TempChannel[i] = initTemp;
      warningSentForChannelTemp[i] = false;
   }

   return HAL_OK;
}

/**
 * @brief Called if an error with batteries is detected. Raises error and stops
 * battery task execution (without tripping watchdog)
 */
void BatteryTaskError()
{
    // Suspend task for now
    ERROR_PRINT("Battery Error occured!\n");
#if IS_BOARD_F7
    // Open AMS contactor. TODO: Maybe remove since we are getting rid of AMS
    // contactor
    AMS_CONT_OPEN;
#endif
    fsmSendEventUrgent(&fsmHandle, EV_HV_Fault, pdMS_TO_TICKS(500));
    TickType_t xLastWakeTime = xTaskGetTickCount();
    while (1) {
        // Suspend this task while still updating watchdog
        watchdogTaskCheckIn(BATTERY_TASK_ID);
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(BATTERY_TASK_PERIOD_MS));
    }
}


/// Maximum number of errors battery task can encounter before reporting error
#define MAX_ERROR_COUNT 5

static uint32_t errorCounter = 0;

/**
 * @brief Called by battery task in an error is encountered that is not
 * immediately fatal. This causes the task to retry its readings/whatever else
 * failed MAX_ERROR_COUNT times, then fail and send error event.
 * TODO: ensure this is max 500 ms to meet rules for cell reading times
 *
 * @return true if errorCounter is below or equal to max error count, false otherwise
 */
bool boundedContinue()
{
    if ((++errorCounter) > MAX_ERROR_COUNT) {
        BatteryTaskError();
        return false;
    } else {
        DEBUG_PRINT("Error counter %d\n", (int)errorCounter);
        watchdogTaskCheckIn(BATTERY_TASK_ID);
        vTaskDelay(pdMS_TO_TICKS(BATTERY_TASK_PERIOD_MS));
        return true;
    }
}

/**
 * @brief Call on a succesful run through main loop.
 * Decrements error counter on succesful run through main loop
 */
void ERROR_COUNTER_SUCCESS()
{
  if (errorCounter > 0)
  {
    errorCounter--;
  }
}


/**
 * Alpha value for cell voltage filter
 * For a 10-90 rise time of 1 sec, set the bandwidth to 0.35 Hz
 * See: https://www.edn.com/electronics-blogs/bogatin-s-rules-of-thumb/4424573/Rule-of-Thumb--1--The-bandwidth-of-a-signal-from-its-rise-time
 * This cuttoff, with a sampling frequency of 100 ms gives an alpha of 0.18
 * See the wikipedia page for the alpha calculation: https://en.wikipedia.org/wiki/Low-pass_filter#Simple_infinite_impulse_response_filter
 */
#define CELL_VOLTAGE_FILTER_ALPHA 0.18



/**
 * @brief This takes an array of the instantaneous cell volages, and filters
 * them on an ongoing basis using the cellVoltagesFiltered array.
 *
 * For check cell voltages, a filtered version of cell voltages is needed to
 * eliminate noise due to bad contact with AMS boards. The hypothesis is that
 * the pogo pins make bad contact with the AMS boards when the motors spin,
 * thus the need for filtering. This hasn't been proven however, but the
 * filtering fixed the errors we saw before (info up to date as of May 2020)
 * NB: Filter voltages shouldn't be sent over CAN, only used with error
 * checking
 *
 * @param[in] cellVoltages Unfiltered cell voltage readings
 * @param[out] cellVoltages Filtered filtered cell voltage readings
 */
void filterCellVoltages(float *cellVoltages, float *cellVoltagesFiltered)
{
    static bool first_run = true;
    if(first_run)
    {
        for(int i = 0; i < NUM_VOLTAGE_CELLS; i++)
        {
            cellVoltagesFiltered[i] = cellVoltages[i];
        }
        first_run = false;
    }
    for (int i = 0; i < NUM_VOLTAGE_CELLS; i++) {
        cellVoltagesFiltered[i] = CELL_VOLTAGE_FILTER_ALPHA*cellVoltages[i]
                                + (1-CELL_VOLTAGE_FILTER_ALPHA)*cellVoltagesFiltered[i];
    }
}

/**
 * @brief Checks cell voltages and temperatures to ensure they are within safe
 * limits, as well as sending out warnings when the values get close to their
 * limits and updating max/min voltages/temps and calculated pack voltage
 *
 * @param[out] maxVoltage The cell voltage of the cell with the max voltage
 * @param[out] minVoltage The cell voltage of the cell with the min voltage
 * @param[out] maxTemp The cell temperature of the cell with the max temperature
 * @param[out] minTemp The cell temperature of the cell with the min temperature
 * @param[out] packVoltage The sum of all the cell voltages, which should be
 * the output voltage of the pack
 *
 * @return HAL_StatusTypeDef
 */
HAL_StatusTypeDef checkCellVoltagesAndTemps(float *maxVoltage, float *minVoltage, float *maxTemp, float *minTemp, float *packVoltage, float* adjustedPackVoltage)
{
   HAL_StatusTypeDef rc = HAL_OK;
   float measure;
   float measure_high;
   float measure_low;
   float currentReading;
   if(getIBus(&currentReading) != HAL_OK){
       ERROR_PRINT("Cannot read current from bus!!");
       sendDTC_FATAL_BMU_ERROR();
       return HAL_ERROR;
   } 
   *maxVoltage = 0;
   *minVoltage = limit_overvoltage;
   *maxTemp = -100; // Cells shouldn't get this cold right??
   *minTemp = CELL_OVERTEMP;
   *packVoltage = 0;
   *adjustedPackVoltage = 0;

   // Unfortunately the thermistors may run slower than the cell voltage measurements
   static uint8_t thermistor_lag_counter = 0;
   enterAdjustedCellVoltages();

   static bool warning_dtc_sent = false;
   for (int i=0; i < NUM_VOLTAGE_CELLS; i++)
   {
      // We have 2 basically confidence measurements
      // We have an adjusted cell measurement which probably overestimates the cell voltage a little at high current
      // We have our standard cell measurement which probably underestimates the cell voltage a little at high current
      measure_high = AdjustedVoltageCell[i];
      measure_low = VoltageCell[i];

      // Check it is within bounds
      if (measure_high < limit_undervoltage) {
         ERROR_PRINT("Cell %d is undervoltage at %f Volts\n", i, measure_high);
         sendDTC_CRITICAL_CELL_VOLTAGE_LOW(i);
         rc = HAL_ERROR;
      } else if (measure_low > limit_overvoltage) {
         ERROR_PRINT("Cell %d is overvoltage at %f Volts\n", i, measure_low);
         sendDTC_CRITICAL_CELL_VOLTAGE_HIGH(i);
         rc = HAL_ERROR;
      } else if (!warning_dtc_sent && measure_high < LIMIT_LOWVOLTAGE_WARNING) {
         ERROR_PRINT("WARN: Cell %d is low voltage at %f Volts\n", i, measure_high);
         sendDTC_WARNING_CELL_VOLTAGE_LOW(i);
         warning_dtc_sent = true;
      }

      // Update max voltage
      if (measure_low > (*maxVoltage)) {(*maxVoltage) = measure_low;}
      if (measure_high < (*minVoltage)) {(*minVoltage) = measure_high;}

      // Sum up cell voltages to get overall pack voltage
      (*adjustedPackVoltage) += measure_high; /*This is our adjusted cell voltage*/
      (*packVoltage) += measure_low;
   }

   if(thermistor_lag_counter >= THERMISTORS_PER_BOARD/NUM_THERMISTOR_MEASUREMENTS_PER_CYCLE)
   {
       for (int i=0; i < NUM_TEMP_CELLS; i++)
       {
            measure = TempChannel[i];
                
            // Check it is within bounds
            if (measure > CELL_OVERTEMP) {
                ERROR_PRINT("Temp Channel %d is overtemp at %f deg C\n", i, measure);
                sendDTC_CRITICAL_CELL_TEMP_HIGH(i);
                rc = HAL_ERROR;
            } else if (measure > CELL_OVERTEMP_WARNING) {
                if (!warningSentForChannelTemp[i]) {
                    ERROR_PRINT("WARN: Temp Channel %d is high temp at %f deg C\n", i, measure);
                    sendDTC_WARNING_CELL_TEMP_HIGH(i);
                    warningSentForChannelTemp[i] = true;
                }
            } else if(measure < CELL_UNDERTEMP){
                ERROR_PRINT("Cell %d is undertemp at %f deg C\n", i, measure);
                sendDTC_CRITICAL_CELL_TEMP_LOW(i);
                rc = HAL_ERROR;
            } else if(measure < CELL_UNDERTEMP_WARNING){
                if(!warningSentForChannelTemp[i]) {
                    ERROR_PRINT("WARN: Cell %d is low temp at %f deg C\n", i, measure);
                    sendDTC_WARNING_CELL_TEMP_LOW(i);
                    warningSentForChannelTemp[i] = true;
                }
            } else if (warningSentForChannelTemp[i] == true) {
                warningSentForChannelTemp[i] = false;
            }

            // Update max voltage
            if (measure > (*maxTemp)) {(*maxTemp) = measure;}
            if (measure < (*minTemp)) {(*minTemp) = measure;}
        }
   }
   else
   {
        thermistor_lag_counter++;
   }

   return rc;
}


/**
 * @brief Calculates the state of power of the battery pack. TODO: this
 * calculation is from 2017, so should be updated for the 2021 pack
 *
 * @return The state of power of the battery pack (in Watts?)
 */
float calculateStateOfPower()
{
   float maxCurrent =  sqrt((((CELL_MAX_TEMP_C - TempCellMax)/CELL_TIME_TO_FAILURE_ALLOWABLE) * (CELL_HEAT_CAPACITY*CELL_MASS))/CELL_DCR);

   return maxCurrent;
}


/**
 * @brief Calculates the state of charge of the battery pack. This is a measure
 * of how much capacity the pack has left on its current charge. Currently its
 * implemented as a simple ratio of min cell voltage versus safe limits
 *
 * @return State of Charge (percent)
 */
float calculateStateOfCharge()
{
    return 100 *((VoltageCellMin - LIMIT_LOWVOLTAGE) / (LIMIT_HIGHVOLTAGE - LIMIT_LOWVOLTAGE));
}

/**
 * @brief Initializes the battery task
 *
 * @return HAL_StatusTypeDef
 */
HAL_StatusTypeDef batteryStart()
{
#if IS_BOARD_F7 && defined(ENABLE_AMS)
    AMS_CONT_CLOSE;
    return batt_init();
#elif IS_BOARD_NUCLEO_F7 || !defined(ENABLE_AMS)
   // For nucleo, cell voltages and temps can be manually changed via CLI for
   // testing, so we don't do anything here
    return HAL_OK;
#else
#error Unsupported board type
#endif
}

/**
 * @brief Publishes the current pack voltage value to the queue for other tasks
 * to read from
 *
 * @param packVoltage The current pack voltage in volts
 *
 * @return HAL_StatusTypeDef
 */
HAL_StatusTypeDef publishPackVoltage(float packVoltage)
{
   xQueueOverwrite(PackVoltageQueue, &packVoltage);

   return HAL_OK;
}

HAL_StatusTypeDef publishAdjustedPackVoltage(float adjustedPackVoltage)
{
   xQueueOverwrite(AdjustedPackVoltageQueue, &adjustedPackVoltage);

   return HAL_OK;
}


/**
 * @brief Gets the current pack voltage. This returns the pack voltage as
 * calculated by summing all the cell voltages.
 *
 * @param packVoltage pointer to a float to return the pack voltage (in volts)
 *
 * @return HAL_StatusTypeDef
 */
HAL_StatusTypeDef getPackVoltage(float *packVoltage)
{
    if (xQueuePeek(PackVoltageQueue, packVoltage, 0) != pdTRUE) {
        ERROR_PRINT("Failed to receive Pack Voltage from queue\n");
        return HAL_ERROR;
    }

    return HAL_OK;
}

HAL_StatusTypeDef getAdjustedPackVoltage(float *adjustedPackVoltage)
{
    if (xQueuePeek(AdjustedPackVoltageQueue, adjustedPackVoltage, 0) != pdTRUE) {
        ERROR_PRINT("Failed to receive Adjusted Pack Voltage from queue\n");
        return HAL_ERROR;
    }

    return HAL_OK;
}

/**
 * @brief Creates the pack voltage queues. Needs to be called before RTOS starts
 *
 * @return HAL_StatusTypeDef
 */
HAL_StatusTypeDef initPackVoltageQueues()
{
   PackVoltageQueue = xQueueCreate(1, sizeof(float));

   if (PackVoltageQueue == NULL) {
      ERROR_PRINT("Failed to create pack voltage queue!\n");
      return HAL_ERROR;
   }
   
   AdjustedPackVoltageQueue = xQueueCreate(1, sizeof(float));

   if (AdjustedPackVoltageQueue == NULL) {
      ERROR_PRINT("Failed to create adjusted pack voltage queue!\n");
      return HAL_ERROR;
   }

   return HAL_OK;
}


/**
 * @brief Sets the maximum current for the charger
 *
 * @param maxCurrent The maximum current, in Amps
 *
 * @return HAL_StatusTypeDef
 */
HAL_StatusTypeDef setMaxChargeCurrent(float maxCurrent)
{
  // Range check, arbitrary max that probable will never need to be changed
  if (maxCurrent <= 0 || maxCurrent >= 100)
  {
    return HAL_ERROR;
  }

  maxChargeCurrent = maxCurrent;

  return HAL_OK;
}

/**
 * @brief Sends the maximum charge current and voltage to the charger and waits
 * for the charger to start charging
 *
 * @return HAL_StatusTypeDef
 */
HAL_StatusTypeDef startCharging()
{
    DEBUG_PRINT("Starting charge\n");
#if IS_BOARD_F7 && defined(ENABLE_CHARGER)
    return startChargerCommunication(maxChargeVoltage,
                                     maxChargeCurrent, BATTERY_TASK_ID);

#endif
    return HAL_OK;
}

/**
 * @brief Sends messages to the charger. The charger expects a message every
 * second so this needs to be repeatedly called during charging
 *
 * @return HAL_StatusTypeDef
 */
HAL_StatusTypeDef continueCharging()
{
#if IS_BOARD_F7 && defined(ENABLE_CHARGER)
   ChargerStatus status;

   sendChargerCommand(maxChargeVoltage, maxChargeCurrent, true /* start charing */);

   if (checkChargerStatus(&status) != HAL_OK) {
      ERROR_PRINT("Failed to get charger status\n");
      return HAL_ERROR;
   }

   if (status.OverallState != CHARGER_OK) {
      ERROR_PRINT("Charger Fail\n");
      sendDTC_FATAL_BMU_Charger_ERROR();
      return HAL_ERROR;
   }

#endif
   return HAL_OK;
}

/**
 * @brief Sends a command to the charger to stop charging
 *
 * @return HAL_StatusTypeDef
 */
HAL_StatusTypeDef stopCharging()
{
    DEBUG_PRINT("stopping charge\n");
#if IS_BOARD_F7 && defined(ENABLE_CHARGER)
    sendChargerCommand(0, 0, false /* stop charing */);
#endif
    return HAL_OK;
}

/**
 * @brief Stops all cells balancing by sending command to AMS boards
 *
 * @return HAL_StatusTypeDef
 */
HAL_StatusTypeDef stopBalance()
{
#if IS_BOARD_F7 && defined(ENABLE_BALANCE)
    batt_unset_balancing_all_cells();
#endif
    
#if IS_BOARD_F7 && defined(ENABLE_AMS) && defined(ENABLE_BALANCE)
    if (batt_write_config() != HAL_OK) {
        return HAL_ERROR;
    }
#endif

    return HAL_OK;
}

/// Array to store if a cell is balancing
bool isCellBalancing[NUM_VOLTAGE_CELLS] = {0};

/**
 * @brief Stops all cells balancing, but stores which cells were balancing to
 * allowing resuming of balance for cells that were balancing
 *
 * @return HAL_StatusTypeDef
 */
HAL_StatusTypeDef pauseBalance()
{
#if IS_BOARD_F7 && defined(ENABLE_BALANCE)
    for (int cell = 0; cell < NUM_VOLTAGE_CELLS; cell++) {
        if (batt_is_cell_balancing(cell)) {
            isCellBalancing[cell] = true;
        } else {
            isCellBalancing[cell] = false;
        }
    }

    if (stopBalance() != HAL_OK) {
        ERROR_PRINT("Failed to stop balance\n");
        return HAL_ERROR;
    }
#endif

    return HAL_OK;
}

/**
 * @brief Resumes balancing after call to @ref pauseBalance
 *
 * @return HAL_StatusTypeDef
 */
HAL_StatusTypeDef resumeBalance()
{
#if IS_BOARD_F7 && defined(ENABLE_BALANCE)
    for (int cell = 0; cell < NUM_VOLTAGE_CELLS; cell++) {
        if (isCellBalancing[cell]) {
            batt_balance_cell(cell);
        }
    }
#endif

#if IS_BOARD_F7 && defined(ENABLE_AMS) && defined(ENABLE_BALANCE)
    if (batt_write_config() != HAL_OK) {
        ERROR_PRINT("Failed to resume balance\n");
    }
#endif

    return HAL_OK;
}

/**
 * @brief Control the balance state of an individual cell. NB: this is
 * inneficient if balancing multiple cells, as he entire balance config is sent
 * to the AMS boards every time this is called. Instead, use the lower level
 * batt_balance_cell/batt_stop_balance_cell to change state for all cells, then
 * call batt_write_config to send the new balance config
 *
 * @param cell The cell to change the balance state of
 * @param set True if should balance cell
 *
 * @return HAL_StatusTypeDef
 */
HAL_StatusTypeDef balance_cell(int cell, bool set)
{
#if IS_BOARD_F7 && defined(ENABLE_BALANCE)
  if (set) batt_balance_cell(cell);
  else batt_stop_balance_cell(cell);
#endif
#if IS_BOARD_F7 && defined(ENABLE_AMS) && defined(ENABLE_BALANCE)
    if (batt_write_config() != HAL_OK) {
        ERROR_PRINT("Failed to resume balance\n");
    }
#endif
    return HAL_OK;
}


/**
 * @brief Maps a value from one range to another in a linear manner
 *
 * @param in The value to map
 * @param low low value of in range
 * @param high high value of in range
 * @param low_out low value of out range
 * @param high_out high value of out range
 *
 * @return in value mapped to out range
 */
float map_range_float(float in, float low, float high, float low_out, float high_out) {
    if (in < low) {
        return low_out;
    } else if (in > high) {
        return high_out;
    }
    float in_range = high - low;
    float out_range = high_out - low_out;

    return (in - low) * out_range / in_range + low_out;
}

/**
 * @brief Calculates the SoC of a cell from the cell's voltage
 *
 * @param cellVoltage The cell voltage
 *
 * @return the SoC of the cell in percent (0-100)
 */
float getSOCFromVoltage(float cellVoltage)
{
    // mV per step 11.88
    float VoltsPerLookup = (limit_overvoltage - limit_undervoltage) / (NUM_SOC_LOOKUP_VALS-1);
    float lookupIndex = (cellVoltage - limit_undervoltage) / VoltsPerLookup;
    if (lookupIndex < 0) { lookupIndex = 0;}
    if (lookupIndex > (NUM_SOC_LOOKUP_VALS-1)) { lookupIndex = (NUM_SOC_LOOKUP_VALS-1);}
    int lookupIndexInt = (int)lookupIndex;

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

/**
 * @brief Performs balance charging.
 *
 * @return @ref ChargeReturn
 */
ChargeReturn balanceCharge(Balance_Type_t using_charger)
{
    // Start charge
    if (using_charger && startCharging() != HAL_OK) {
        return CHARGE_ERROR;
    }

    bool balancingCells = false; // Are we balancing any cell currently?
    uint32_t lastBalanceCheck = 0;
    bool waitingForBalanceDone = false; // Set to true when receive stop but still balancing
    uint32_t dbwTaskNotifications;
    float packVoltage;
    float adjustedPackVoltage;

    while (1) {
       /*
        * Need to send msg to charger every second to continue charging
        */
       if (using_charger && !waitingForBalanceDone) {
              DEBUG_PRINT("Still Charging\n");
          if (continueCharging() != HAL_OK) {
             ERROR_PRINT("Failed to send charge continue message\n");
             if (boundedContinue()) { continue; }
          }
       }

        /*
         * Perform cell reading, need to pause any ongoing balance in order to
         * get good voltage readings
         * After we have read, we can re-enable balancing on cells
         */
        if (pauseBalance() != HAL_OK) {
            ERROR_PRINT("Failed to pause balance!\n");
            if (boundedContinue()) { continue; }
        }

        // Check in before delay
        watchdogTaskCheckIn(BATTERY_TASK_ID);
        if (CELL_RELAXATION_TIME_MS >= BATTERY_CHARGE_TASK_PERIOD_MS) {
            ERROR_PRINT("Cell relaxation time %d > task period %d",
                        CELL_RELAXATION_TIME_MS, BATTERY_CHARGE_TASK_PERIOD_MS);
            BatteryTaskError();
        } else {
            vTaskDelay(pdMS_TO_TICKS(CELL_RELAXATION_TIME_MS));
        }

        if (readCellVoltagesAndTemps() != HAL_OK) {
            BatteryTaskFailure = READ_CELL_VOLTAGE_TEMPS_FAIL_BIT;
            sendCAN_BMU_BatteryChecks();
            ERROR_PRINT("Failed to read cell voltages and temperatures!\n");
            BatteryTaskError();
        }

#if IS_BOARD_F7 && defined(ENABLE_AMS)
        if (checkForOpenCircuit() != HAL_OK) {
            ERROR_PRINT("Open wire test failed!\n");
            BatteryTaskError();
        }
#endif

        if (resumeBalance() != HAL_OK) {
            ERROR_PRINT("Failed to resume balance!\n");
            if (boundedContinue()) { continue; }
        }


        /*
         * Safety checks for cells
         */
        if (checkCellVoltagesAndTemps(
                ((float *)&VoltageCellMax), ((float *)&VoltageCellMin),
                ((float *)&TempCellMax), ((float *)&TempCellMin),
                &packVoltage, &adjustedPackVoltage) != HAL_OK)
        {
            BatteryTaskError();
        }

        /*
         * Check if we should balance any cells
         * Only balance above a minimum voltage
         */
        if (VoltageCellMin >= BALANCE_START_VOLTAGE || !using_charger)
        {
            if (xTaskGetTickCount() - lastBalanceCheck
                > pdMS_TO_TICKS(BALANCE_RECHECK_PERIOD_MS))
            {
                balancingCells = false;

                /*DEBUG_PRINT("Starting balance\n");*/
                /*DEBUG_PRINT("Voltages:\n");*/
                /*for (int cell = 0; cell < NUM_VOLTAGE_CELLS; cell++) {*/
                    /*DEBUG_PRINT("%d: %f,", cell, VoltageCell[cell]);*/
                /*}*/
                /*DEBUG_PRINT("\n");*/
                float minCellSOC = getSOCFromVoltage(VoltageCellMin);
                float maxCellSOC = getSOCFromVoltage(VoltageCellMax);
                DEBUG_PRINT("Voltage min %f (SOC %f), max %f (SOC %f)\n\n", VoltageCellMin, minCellSOC, VoltageCellMax, maxCellSOC);
                for (int cell=0; cell < NUM_VOLTAGE_CELLS; cell++) {
                    float cellSOC = getSOCFromVoltage(AdjustedVoltageCell[cell]);
                    watchdogTaskCheckIn(BATTERY_TASK_ID);
                    /*DEBUG_PRINT("Cell %d SOC: %f\n", cell, cellSOC);*/

                    if (cellSOC - minCellSOC > BALANCE_MIN_SOC_DELTA) {
                        DEBUG_PRINT("Balancing cell %d\n", cell);
#if IS_BOARD_F7
                        batt_balance_cell(cell);
#endif
                        balancingCells = true;
                    } else {
                      DEBUG_PRINT("Not balancing cell %d\n", cell);
#if IS_BOARD_F7
                      batt_stop_balance_cell(cell);
#endif
                    }
                }

                DEBUG_PRINT("\n\n\n");

#if IS_BOARD_F7 && defined(ENABLE_AMS)
                batt_set_disharge_timer(DT_30_SEC);
                if (batt_write_config() != HAL_OK)
                {
                    return CHARGE_ERROR;
                }
#endif

                lastBalanceCheck = xTaskGetTickCount();
            }
        } else {
            balancingCells = false;
            DEBUG_PRINT("Can't balance cells as VoltageCellMin (%f) < BALANCE_START_VOLTAGE (%f)\n", VoltageCellMin, BALANCE_START_VOLTAGE);
            if (stopBalance() != HAL_OK) {
                ERROR_PRINT("Failed to stop balance\n");
                if (boundedContinue()) { continue; }
            }
        }

        /*
         * Check if we are done charging/balancing
         */
        if (using_charger && getSOCFromVoltage(VoltageCellMin) >= CHARGE_STOP_SOC && !balancingCells) {
            DEBUG_PRINT("Done charging\n");
            if (using_charger && stopCharging() != HAL_OK) {
                return CHARGE_ERROR;
            }
            break;
        }

        /*
         * Check if we should stop charge mode
         */
        BaseType_t rc = xTaskNotifyWait( 0x00, /* Don't clear any notification bits on entry. */
                         UINT32_MAX, /* Reset the notification value to 0 on exit. */
                         &dbwTaskNotifications, /* Notified value pass out in
                                                   dbwTaskNotifications. */
                         0);                    /* Timeout */

        if (rc == pdTRUE) {
            if (dbwTaskNotifications & (1<<BATTERY_STOP_NOTIFICATION)) {
                DEBUG_PRINT("Stopping charge\n");
                if (using_charger && stopCharging() != HAL_OK) {
                    stopBalance();
                    return CHARGE_ERROR;
                }
                else if (!using_charger) // CLI command to stop was called
                {
                    stopBalance();
                    return CHARGE_STOPPED;
                }
                else if (balancingCells) {
                    DEBUG_PRINT("Balance ongoing, waiting for finish\n");
                    waitingForBalanceDone = true;
                } else {
                    DEBUG_PRINT("Not balancing, can stop safely\n");
                    return CHARGE_DONE;
                }
            } else if (dbwTaskNotifications & (1<<CHARGE_START_NOTIFICATION)) {
                DEBUG_PRINT("Received charge start, but already charging\n");
            } else {
                DEBUG_PRINT("Received invalid notification\n");
            }
        }

        /*
         * Check if we are waiting for balance to finish before stop
         */
        if (waitingForBalanceDone && !balancingCells) {
            DEBUG_PRINT("Balance ended, stopping charge\n");
            return CHARGE_STOPPED;
        }

        StateBatteryPowerHV = calculateStateOfPower();
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
            if (boundedContinue()) { continue; }
        }

        publishPackVoltage(packVoltage);
        publishAdjustedPackVoltage(adjustedPackVoltage);

        // Succesfully reach end of loop, update error counter to reflect that
        ERROR_COUNTER_SUCCESS();
        /* !!! Change the check in in bounded continue as well if you change
         * this */
        watchdogTaskCheckIn(BATTERY_TASK_ID);
        vTaskDelay(pdMS_TO_TICKS(BATTERY_CHARGE_TASK_PERIOD_MS));
    }

    return CHARGE_DONE;
}

/**
 * @brief Task to monitor cell voltages and temperatures, as well as perform
 * balance charging
 */
void batteryTask(void *pvParameter)
{
    if (initVoltageAndTempArrays() != HAL_OK)
    {
       Error_Handler();
    }


#if IS_BOARD_F7 && defined(ENABLE_AMS)
    HAL_StatusTypeDef ret = HAL_ERROR;
    for(int num_tries = 0; num_tries < START_NUM_TRIES; num_tries++)
    {
        ret = batteryStart();
        if (ret == HAL_OK)
        {
            break;
        }
    }
    if(ret != HAL_OK)
    {
        BatteryTaskFailure = BATTERY_START_FAIL_BIT;
        sendCAN_BMU_BatteryChecks();
        BatteryTaskError();
    }
#endif

    if (registerTaskToWatch(BATTERY_TASK_ID, 2*pdMS_TO_TICKS(BATTERY_TASK_PERIOD_MS), false, NULL) != HAL_OK)
    {
        ERROR_PRINT("Failed to register battery task with watchdog!\n");
        Error_Handler();
    }

    TickType_t xLastWakeTime = xTaskGetTickCount();

    float packVoltage;
    float adjustedPackVoltage;
    uint32_t dbwTaskNotifications;
    while (1)
    {
        /*
         * Check if we should start charging
         */
        BaseType_t rc = xTaskNotifyWait( 0x00, /* Don't clear any notification bits on entry. */
                         UINT32_MAX, /* Reset the notification value to 0 on exit. */
                         &dbwTaskNotifications, /* Notified value pass out in
                                                   dbwTaskNotifications. */
                         0);                    /* Timeout */

        if (rc == pdTRUE) {
            if (dbwTaskNotifications & ((1<<CHARGE_START_NOTIFICATION) | (1<<BALANCE_START_NOTIFICATION))) {
                
                if (dbwTaskNotifications & (1<<CHARGE_START_NOTIFICATION))
                {
                    ChargeEN_State = ChargeEN_State_On;
                    sendCAN_BMU_ChargeEN_State();
                }
                if (HAL_OK != watchdogTaskChangeTimeout(BATTERY_TASK_ID,
                                                        2*BATTERY_CHARGE_TASK_PERIOD_MS))
                {
                    ERROR_PRINT("Failed to change watchdog timeout for battery task\n");
                } else {
                    ChargeReturn chargeRc;
                    
                    if (dbwTaskNotifications & (1<<CHARGE_START_NOTIFICATION))
                    {
                        chargeRc = balanceCharge(USING_CHARGER);
                    }
                    else if (dbwTaskNotifications & (1<<BALANCE_START_NOTIFICATION))
                    {
                        chargeRc = balanceCharge(USING_CLI);
                    }
                    else
                    {
                        ERROR_PRINT("Processing unknown notification in batteryTask\n");
                        chargeRc = CHARGE_ERROR;
                    }

                    if (HAL_OK != watchdogTaskChangeTimeout(BATTERY_TASK_ID,
                                                            2*BATTERY_TASK_PERIOD_MS))
                    {
                        ERROR_PRINT("Failed to change watchdog timeout for battery task\n");
                    }

                    if (chargeRc == CHARGE_ERROR) {
                        ERROR_PRINT("Failed to balance charge\n");
                        fsmSendEvent(&fsmHandle, EV_Charge_Error, portMAX_DELAY);
                    } else if (chargeRc == CHARGE_DONE) {
                        DEBUG_PRINT("Finished charge/balancing\n");
                        fsmSendEvent(&fsmHandle, EV_Notification_Done, 20);
                    } else if (chargeRc == CHARGE_STOPPED) {
                        DEBUG_PRINT("Stopped charge/balancing\n");
                        fsmSendEvent(&fsmHandle, EV_Notification_Stop, 20);
                    } else {
                        ERROR_PRINT("Unkown charge return code %d\n", chargeRc);
                        fsmSendEvent(&fsmHandle, EV_Charge_Error, portMAX_DELAY);
                    }
                }
                if (dbwTaskNotifications & (1<<CHARGE_START_NOTIFICATION))
                {
                    ChargeEN_State = ChargeEN_State_Off;
                    sendCAN_BMU_ChargeEN_State();
                }
            }
            else if (dbwTaskNotifications & (1<<BATTERY_STOP_NOTIFICATION)) {
                DEBUG_PRINT("Received charge stop, but not charging\n");
            } else {
                DEBUG_PRINT("Received invalid notification\n");
            }
        }
#if IS_BOARD_F7 && defined(ENABLE_AMS)
        if (checkForOpenCircuit() != HAL_OK) {
            BatteryTaskFailure = OPEN_CIRCUIT_FAIL_BIT;
            sendCAN_BMU_BatteryChecks();
            ERROR_PRINT("Open wire test failed!\n");
            if (boundedContinue()) { continue; }
        }
#endif

#if IS_BOARD_F7 && defined(ENABLE_AMS)
        if (readCellVoltagesAndTemps() != HAL_OK) {
            BatteryTaskFailure = READ_CELL_VOLTAGE_TEMPS_FAIL_BIT;
            sendCAN_BMU_BatteryChecks();
            ERROR_PRINT("Failed to read cell voltages and temperatures!\n");
            if (boundedContinue()) { continue; }
        }
#endif
        if (checkCellVoltagesAndTemps(
              ((float *)&VoltageCellMax), ((float *)&VoltageCellMin),
              ((float *)&TempCellMax), ((float *)&TempCellMin),
              &packVoltage, &adjustedPackVoltage) != HAL_OK)
        {
            BatteryTaskFailure = CHECK_CELL_VOLTAGE_TEMPS_FAIL_BIT;
            sendCAN_BMU_BatteryChecks();
            ERROR_PRINT("Failed check of battery cell voltages and temps\n");
            if (boundedContinue()) { continue; }
        }

        if (publishPackVoltage(packVoltage) != HAL_OK) {
            BatteryTaskFailure = PACK_VOLTAGE_FAIL_BIT;
            sendCAN_BMU_BatteryChecks();
            ERROR_PRINT("Failed to publish pack voltage\n");
            if (boundedContinue()) { continue; }
        }
        // Adjusted Pack Voltage not critical
        publishAdjustedPackVoltage(adjustedPackVoltage);

        StateBatteryPowerHV = calculateStateOfPower();
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
            ERROR_PRINT("Failed to send battery status HV\n");
            if (boundedContinue()) { continue; }
        }

        static bool released_soc = false;
        if(!released_soc)
        {
            xTaskNotifyGive(stateOfChargeHandle);
            released_soc = true;
        }
        // Succesfully reach end of loop, update error counter to reflect that
        ERROR_COUNTER_SUCCESS();
        /*!!! Change the check in in bounded continue as well if you change
         * this */
        watchdogTaskCheckIn(BATTERY_TASK_ID);
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(BATTERY_TASK_PERIOD_MS));
    }
}

/*
 * CAN Send task for cell voltages and temperatures
 */

/// The period to send cell voltage and temperature CAN messages
#define CAN_CELL_SEND_PERIOD_MS 50
#define CAN_CELL_SEND_TASK_ID 8

bool sendOneCellVoltAndTemp = false;
int cellToSend;

/**
 * @brief Repeatedly send the cell voltage and temperature for a specific cell.
 * This is instead of the normal behaviour which cycles through all the cells
 * sending a message every @ref CAN_CELL_SEND_PERIOD_MS. Useful for battery
 * testing to monitor a cell at higher frequency
 *
 * @param cellIdx The cell to repeatedly send (note, this uses firmwares 0
 * based indexing, instead of electricals 1 based indexing of cell numbers)
 */
void setSendOnlyOneCell(int cellIdx)
{
  sendOneCellVoltAndTemp = true;
  cellToSend = cellIdx;
}

/**
 * @brief Stop sending only one cell repeatedly, and go back to sending all
 * cells over CAN
 */
void clearSendOnlyOneCell()
{
  sendOneCellVoltAndTemp = false;
}

// Round up input to the nearest multiple. 
// ex. roundUpNearestMultiple(7, 100) = 100
//     roundUpNearestMultiple(117, 100) = 200
//     roundUpNearestMultiple(70, 3) = 72 (our use case for cell indexing)
static inline uint8_t roundUpNearestMultiple(uint8_t input, uint8_t multiple)
{
    return (multiple <= 1 || input % multiple == 0) ? input : input + (multiple - (input % multiple));
}

/**
 * @brief Sends the cell voltages and temperatures over CAN
 *
 */
void canSendCellTask(void *pvParameters)
{

    uint32_t cellIdxToSend = 0;
    TickType_t xLastWakeTime = xTaskGetTickCount();

    if (registerTaskToWatch(CAN_CELL_SEND_TASK_ID, 5*pdMS_TO_TICKS(CAN_CELL_SEND_PERIOD_MS), false, NULL) != HAL_OK)
    {
        ERROR_PRINT("ERROR: Failed to register canSendCellTask with watchdog\n");
    }
    while (1) {
        if (sendOneCellVoltAndTemp) {
            // The cell index for sending should be a multiple of 3, as the cells are
            // sent in groups of 3
            cellIdxToSend = cellToSend - (cellToSend % CAN_TX_CELL_GROUP_LEN);
        }

        sendCAN_BMU_CellVoltage(cellIdxToSend);
        vTaskDelay(2); // Added to prevent CAN mailbox full
        sendCAN_BMU_CellVoltage_Adjusted(cellIdxToSend);
        vTaskDelay(2); // Added to prevent CAN mailbox full
        sendCAN_BMU_ChannelTemp(cellIdxToSend);

        // Move on to next cells
        // 3 Cells per CAN message
        cellIdxToSend += CAN_TX_CELL_GROUP_LEN;
        if(cellIdxToSend >= NUM_VOLTAGE_CELLS)
        {
            cellIdxToSend = 0;
        }
        watchdogTaskCheckIn(CAN_CELL_SEND_TASK_ID);
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(CAN_CELL_SEND_PERIOD_MS));
    }
}
