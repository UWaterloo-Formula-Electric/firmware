// mirrored from bmu/Inc/F7_Inc/ltc_chip.h (can't include it here, it pulls in the STM32 HAL)
#define NUM_VOLTAGE_CELLS 140
#define NUM_TEMP_CELLS 140
#define BMU_CELLVOLTAGE 0x18800401
#define BMU_CELLVOLTAGE_ADJUSTED 0x18840401
#define BMU_CHANNELTEMP 0x18C00401
#define BMU_STATEBUSHV 0x08101201
#define BMU_BATTERYSTATUSHV 0x08041001
#define BMU_AMSVBATT 0x08010C01

typedef enum {
    SELF_CHECK,
    WAIT_SYSTEM_UP,
    HV_DISABLE,
    HV_ENABLE,
    PRECHARGE,
    DISCHARGE,
    CHARGING,
    FAILURE_FATAL,
    FAILURE_CBRB_DISABLED,
    FAILURE_CBRB_ENABLED,
    BALANCING
} BMU_States_t;

typedef struct {
    float voltage[NUM_VOLTAGE_CELLS];
    float voltageAdjusted[NUM_VOLTAGE_CELLS];
    float temperature[NUM_TEMP_CELLS];
    uint64_t cell_rx_ms[NUM_VOLTAGE_CELLS];
    uint64_t temp_rx_ms[NUM_TEMP_CELLS];

    float voltageBusHV;
    float voltageCellMax;
    float voltageCellMin;
    float currentBusHV;
    uint64_t bus_rx_ms;

    float StateBatteryChargeHV;
    float StateBatteryHealthHV;
    float StateBatteryPowerHV;
    float TempCellMax;
    float TempCellMin;
    BMU_States_t StateBMS;
    int HighestTempThermistor;
    uint64_t status_rx_ms;

    float AMS_PackVoltage;
} state_t;