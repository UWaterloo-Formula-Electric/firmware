#ifndef BATTERIES_H

#define BATTERIES_H

#include "FreeRTOS.h"
#include "queue.h"
#include "bsp.h"

/*
 * Battery task Defines and Variables
 */

// This is subject to change and is expected to be 100ms
#define BATTERY_TASK_PERIOD_MS 100
#define BATTERY_CHARGE_TASK_PERIOD_MS 500

/**
 * @defgroup CellConfig
 *
 * Used for various safety checks and State of Charge calculation.
 * Update these values for new cells.
 *
 * @{
 */

/* The following is specified in Volts (floating point) */
/// Maximum voltage of a cell, will send a critical DTC is exceeded.
#define DEFAULT_LIMIT_OVERVOLTAGE 4.2F
/// Used in SOC function. TODO: confirm this value
#define LIMIT_HIGHVOLTAGE 4.2F
/// Used in SOC function. TODO: confirm this value
#define LIMIT_LOWVOLTAGE 3.0F
/// Minimum voltage of a cell, will send a critical DTC if it goes below
#define DEFAULT_LIMIT_UNDERVOLTAGE 2.5F
/// Warning voltage of a cell, will send a warning DTC if it goes below
#define LIMIT_LOWVOLTAGE_WARNING 2.8F
/// Rate at which the low voltage threshold dynamically lowers vs current
#define LIMIT_LOWVOLTAGE_WARNING_SLOPE 0.0043125F

/* The following values are used in State of Power calculation and should
 * be determined from cell testing data */

// TODO: Update these values for 2021 cells
#define CELL_TIME_TO_FAILURE_ALLOWABLE (6.0)    ///< seconds?
#define CELL_DCR (0.01)                         ///< Ohms
#define CELL_HEAT_CAPACITY (1034.2)             ///< kJ/kg*K
#define CELL_MASS (0.496)                       ///< kg

// A constant which defines how much we adjust our AdjustedCellVoltage factoring in the cell's Internal Resistance
// This is a very conservative number of 3mOhms. This is not the measured cell internal resistance.
// Our current pack is 70s7p. So this assumption factors in that IBus is total current from cells and the current gets divided by 7
#define ADJUSTED_CELL_IR_DEFAULT (0.00486F)

/** Maximum allowable cell temperature, will send critical DTC if surpassed */
#define CELL_OVERTEMP (CELL_MAX_TEMP_C)
/** Temp at warning DTC is sent */
#define CELL_OVERTEMP_WARNING (CELL_MAX_TEMP_C - 10)
/** Similar to @ref CELL_OVERTEMP, minimum temp before sending critical DTC */
#define CELL_UNDERTEMP 0
/** Similar to @ref CELL_OVERTEMP_WARNING, temp will send warning DTC */
#define CELL_UNDERTEMP_WARNING 5

/** @} Cell Characteristics */

/*
 * Charging constants
 */

/// Block balancing below this cell voltage
#define BALANCE_START_VOLTAGE (3.7F)

/**
 * Threshold to begin balancing a cell when it's SoC is this percent higher
 * than the minimum cell SoC in the entire pack
 */
#define BALANCE_MIN_SOC_DELTA (1.0F)

/// Pause balancing for this length when reading cell voltages to get good readings
#define CELL_RELAXATION_TIME_MS (250)

/// SoC to stop charging at (of the cell with lowest SoC)
#define CHARGE_STOP_SOC (98.0)

/**
 * If using charge cart heartbeat, this heartbeat timeout. NB: We are phasing
 * out use of charge cart as a board with a microcontroller
 */
#define CHARGE_CART_HEARTBEAT_MAX_PERIOD (1000)

/// Default charging current limit (Amps)
#define CHARGE_DEFAULT_MAX_CURRENT 5

/**
 * Period at which cell SoCs are checked to determine which cells to balance.
 * This should be long enough so cells aren't constantly being toggled
 * between balance and not
 */
#define BALANCE_RECHECK_PERIOD_MS (3000)
#define START_NUM_TRIES (3)

#define BATTERY_START_FAIL_BIT                      (1U << 0)
#define OPEN_CIRCUIT_FAIL_BIT                       (1U << 1)
#define READ_CELL_VOLTAGE_TEMPS_FAIL_BIT            (1U << 2)
#define CHECK_CELL_VOLTAGE_TEMPS_FAIL_BIT           (1U << 3)
#define PACK_VOLTAGE_FAIL_BIT                       (1U << 4)

/* 
 * canSendTask sends in multiples of 3 as 8 bits must be used for the muxIndex leaving 56 data bits for cell readings of 16 bits each
 * Therefore cell readings are send in groups of 3.
 */

#define CAN_TX_CELL_GROUP_LEN 3

/**
 * Return of balance charge function
 */
typedef enum ChargeReturn
{
    CHARGE_DONE,    ///< Charging finished, cells reached fully charged
    CHARGE_STOPPED, ///< Charging was stopped as requested, cells not fully charged
    CHARGE_ERROR    ///< Error occured stopping charging, cells not fully charged
} ChargeReturn;

// Used by FAN Control to determine when to turn on fans
#define CELL_MAX_TEMP_C (60.0)

typedef enum Balance_Type_t {
    USING_CLI,
    USING_CHARGER
} Balance_Type_t;

typedef enum Charge_Notifications_t {
    CHARGE_START_NOTIFICATION,
    BALANCE_START_NOTIFICATION,
    BATTERY_STOP_NOTIFICATION,                  
} Battery_Notifications_t;

HAL_StatusTypeDef getIBus(float *IBus);
HAL_StatusTypeDef getVBatt(float *VBatt);
HAL_StatusTypeDef getVBus(float *VBus);

HAL_StatusTypeDef initBusVoltagesAndCurrentQueues();
HAL_StatusTypeDef balance_cell(int cell, bool set);
HAL_StatusTypeDef getPackVoltage(float *packVoltage);
HAL_StatusTypeDef getAdjustedPackVoltage(float *packVoltage);
HAL_StatusTypeDef initPackVoltageQueues();
float map_range_float(float in, float low, float high, float low_out, float high_out);
HAL_StatusTypeDef setMaxChargeCurrent(float maxCurrent);
void setSendOnlyOneCell(int cellIdx);
void clearSendOnlyOneCell();
HAL_StatusTypeDef cliSetVBatt(float VBatt);
HAL_StatusTypeDef cliSetVBus(float VBus);
HAL_StatusTypeDef cliSetIBus(float IBus);
HAL_StatusTypeDef publishBusVoltage(float *pVBus);
HAL_StatusTypeDef publishBattVoltage(float *pVBatt);
HAL_StatusTypeDef publishBusCurrent(float *pIBus);
void cliSetStateBusHVSendPeriod(uint32_t period);
uint32_t cliGetStateBusHVSendPeriod();
#endif /* end of include guard: BATTERIES_H */
