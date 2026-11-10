
#include "ltc_chip.h"
#include "bsp.h"
#include "math.h"
#include "stdbool.h"
#include "errorHandler.h"
#include "debug.h"
#include "FreeRTOS.h"
#include "task.h"
#include <string.h>
#include "ltc_chip_interface.h"
#include "batteries.h"

#define OPEN_WIRE_IBUS_TOLERANCE_A (10.0f)
#define OPEN_WIRE_MIN_DIVISOR_V (0.0002f)

// Waits out a single shot S-ADC conversion. vTaskDelay can return up to a tick early, so wait one tick more
static void waitForSAdcConversion(void)
{
    vTaskDelay(pdMS_TO_TICKS(SADC_CONVERSION_DELAY_MS) + 1);
}

/*
 * Starts a conversion, first setting the results it writes to 0x8000. A chip that misses the conversion command then
 * reads back 0x8000, which the reads reject, instead of its previous result with a valid PEC. ADCV and ADSV reset their
 * results themselves once received, but ADAX doesn't
 */
static HAL_StatusTypeDef clearAndStartConversion(ltc_command_t conversion)
{
#if LTC_CHIP == ADBMS_CHIP_6830B
    HAL_StatusTypeDef rc = HAL_OK;
    switch (conversion) {
        case ADCV:
            rc = batt_broadcast_command(CLEAR_CELL_RESULTS);
            break;
        case ADCV_RD:
            // Writes the S-ADC results too
            rc = batt_broadcast_command(CLEAR_CELL_RESULTS);
            if (rc == HAL_OK) {
                rc = batt_broadcast_command(CLEAR_S_RESULTS);
            }
            break;
        case ADSV:
        case ADSV_OW_EVEN:
        case ADSV_OW_ODD:
            rc = batt_broadcast_command(CLEAR_S_RESULTS);
            break;
        case ADAX_UP:
        case ADAX_DOWN:
        case ADAX_ITMP:
            rc = batt_broadcast_command(CLEAR_AUX_RESULTS);
            break;
        default:
            break;
    }
    if (rc != HAL_OK) {
        return rc;
    }
#endif
    return batt_broadcast_command(conversion);
}

#if LTC_CHIP == ADBMS_CHIP_6830B && CS_DIFF_LOG_ENABLED
// Averaged C-ADC minus S-ADC of each cell from the redundant readings, see batt_get_cs_diff
static float csDiffLatest[NUM_VOLTAGE_CELLS];
static float csDiffLowest[NUM_VOLTAGE_CELLS];
static float csDiffHighest[NUM_VOLTAGE_CELLS];
static uint32_t csDiffSamples = 0;

// After a redundant ADCV (RD = 1) the chip compares the averaged C-ADC results (8 conversions) with the S-ADC results,
// so log the same two. Information only: a failed read just skips this reading
static void logCsDiff(void)
{
    static float acVoltages[NUM_VOLTAGE_CELLS];
    static float sVoltages[NUM_VOLTAGE_CELLS];
    if (batt_read_averaged_cell_voltages(acVoltages) != HAL_OK || batt_read_ADSV(sVoltages) != HAL_OK) {
        return;
    }
    for (int cell = 0; cell < NUM_VOLTAGE_CELLS; cell++) {
        const float diff = acVoltages[cell] - sVoltages[cell];
        csDiffLatest[cell] = diff;
        if (csDiffSamples == 0 || diff < csDiffLowest[cell]) {
            csDiffLowest[cell] = diff;
        }
        if (csDiffSamples == 0 || diff > csDiffHighest[cell]) {
            csDiffHighest[cell] = diff;
        }
    }
    csDiffSamples++;
}
#endif

#if LTC_CHIP == ADBMS_CHIP_6830B
// Chips whose balancing is held off after a thermal shutdown, and since when (batt_recover_reset_chips)
static bool balanceHeldOff[NUM_DEVICES];
static TickType_t balanceHeldOffTick[NUM_DEVICES];

static bool isBalanceHeldOff(int board, int chip)
{
    const int dev = board * NUM_LTC_CHIPS_PER_BOARD + chip;
    if (balanceHeldOff[dev]
        && xTaskGetTickCount() - balanceHeldOffTick[dev] >= pdMS_TO_TICKS(THERMAL_SHUTDOWN_BALANCE_HOLD_OFF_MS)) {
        balanceHeldOff[dev] = false;
        ERROR_PRINT("AMS board %d chip %d: balancing allowed again after its thermal shutdown\n", board, chip);
    }
    return balanceHeldOff[dev];
}
#endif

HAL_StatusTypeDef batt_init()
{

    batt_init_chip_configs();
    batt_init_chip_configs_pwm();
    if (batt_spi_wakeup(true) != HAL_OK) {
        ERROR_PRINT("Failed to wake up boards\n");
        return HAL_ERROR;
    }

    if(batt_write_config() != HAL_OK) {
    	ERROR_PRINT("Failed to write batt config to boards\n");
        return HAL_ERROR;
    }
    
    batt_spi_wakeup(false);

	if(batt_verify_config() != HAL_OK){
		ERROR_PRINT("Failed to read batt config from boards\n");
		return HAL_ERROR;
	}

#if LTC_CHIP == ADBMS_CHIP_6830B
    // The C-ADC vs S-ADC, OV/UV and SLEEP flags are all set after power up or sleep. batt_verify_config has already
    // waited for the reference, and the clears only work once it's up
    batt_spi_wakeup(false);
    if (batt_clear_cs_flags() != HAL_OK || batt_clear_ovuv_flags() != HAL_OK || batt_clear_reset_flags() != HAL_OK) {
        ERROR_PRINT("Failed to clear the AMS diagnostic flags\n");
        return HAL_ERROR;
    }
#endif

    if(batt_start_ADC_conversion() != HAL_OK){
        ERROR_PRINT("Failed to start ADC conversion\n");
		return HAL_ERROR;
    }
    
    return HAL_OK;
}

HAL_StatusTypeDef batt_start_ADC_conversion(void)
{
    if (batt_spi_wakeup(false /* not sleeping*/))
    {
        return HAL_ERROR;
    }
    
    batt_spi_wakeup(false);

    if (batt_broadcast_command(ADCV) != HAL_OK) {
        return HAL_ERROR;
    }
    
    return HAL_OK;
}

HAL_StatusTypeDef batt_read_cell_voltages(float *cell_voltage_array)
{
    if (batt_spi_wakeup(false /* not sleeping*/))
    {
        return HAL_ERROR;
    }

#if LTC_CHIP == ADBMS_CHIP_6830B
    // Redundant measurement: the chips also compare each cell's C-ADC and S-ADC results, see
    // batt_check_diagnostic_flags
    if (clearAndStartConversion(ADCV_RD) != HAL_OK) {
        return HAL_ERROR;
    }
    waitForSAdcConversion();
#else
    if (batt_broadcast_command(ADCV) != HAL_OK) {
        return HAL_ERROR;
    }
    delay_us(VOLTAGE_MEASURE_DELAY_US);
#endif

    if (batt_spi_wakeup(false /* not sleeping*/))
    {
        return HAL_ERROR;
    }

    if (batt_readBackCellVoltage(cell_voltage_array, POLL_VOLTAGE) != HAL_OK)
    {
        return HAL_ERROR;
    }

#if LTC_CHIP == ADBMS_CHIP_6830B && CS_DIFF_LOG_ENABLED
    logCsDiff();
#endif

    return HAL_OK;
}

HAL_StatusTypeDef batt_read_cell_temps_single_channel(uint8_t channel, float *cell_temp_array)
{
    if (batt_spi_wakeup(false /* not sleeping*/))
    {
        return HAL_ERROR;
    }

    // Validate parameters
    if(c_assert(channel < TEMP_CHANNELS_PER_BOARD))
    {
        DEBUG_PRINT("Temp channel %u index out of range\r\n", channel);
        return HAL_ERROR;
    }

	batt_set_temp_config(channel);

    if (batt_write_config() != HAL_OK)
    {
        ERROR_PRINT("Failed to setup mux for temp reading\n");
        return HAL_ERROR;
    }

    delay_us(MUX_MEASURE_DELAY_US);
    
    if (batt_spi_wakeup(false /* not sleeping*/))
    {
        return HAL_ERROR;
    }

    #if LTC_CHIP == LTC_CHIP_6804 || LTC_CHIP == LTC_CHIP_6812
    if (batt_broadcast_command(ADAX) != HAL_OK) {
        return HAL_ERROR;
    }
    #elif LTC_CHIP == ADBMS_CHIP_6830B
    if (clearAndStartConversion(ADAX_DOWN) != HAL_OK) {
        return HAL_ERROR;
    }
    #endif
    delay_us(TEMP_MEASURE_DELAY_US);

    if (batt_spi_wakeup(false /* not sleeping*/))
    {
        return HAL_ERROR;
    }

    if (batt_read_thermistors(channel, cell_temp_array) != HAL_OK)
    {
        ERROR_PRINT("Failed to read temp adc vals\n");
        return HAL_ERROR;
    }

    return HAL_OK;
}

/*
              (Top of board #1) 
        X  15                   X  0
        X  14                   X  1
        X  13                   X  2
        X  12                   X  3
        X  11                   X  4
        X  10                   X  5
        X  9                    X  6
        NC 8                    NC 7


              (Top of board #2)  
        X  15                   X  0
        X  14                   X  1
        X  13                   X  2
        X  12                   X  3
        X  11                   X  4
        X  10                   X  5
        X  9                    NC 6
        NC 8                    NC 7

The images above represent the thermistor connections of the first and second AMS boards in each segment. The images are orineted upright (the text on the PCB will be correctly oriented).
An 'X' represents a connected thermistor, and 'NC' represents no thermistor connection.
On the actual connectors, a thermistor connection will require two adjacent pins, but this has been simplified to a sinlge 'X'.
Thermistors #7 and #8 are not connected on either board, allowing us to skip these readings all together.
Thermistor #6 is only connected on board 1, meaning we must still read it from both boards, but will parse the readings to only store the values from the first board in each segment.
The parsing and mapping of thermistors to their correct indices is done in the "batt_read_thermistors" function.
Differ any questions to Justin Vuong, Owen Brake or Andrew Stekar.

Future todo: could add a reading of VREF2 to get a better estimate of thermistor resistance
*/
HAL_StatusTypeDef batt_read_cell_temps(float *cell_temp_array)
{
    uint8_t channel_read_order[14] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13};
    static uint8_t curr_channel_read_index = 0;
    for (int i = 0; i < NUM_THERMISTOR_MEASUREMENTS_PER_CYCLE; i++)
    {
        if (batt_read_cell_temps_single_channel(channel_read_order[curr_channel_read_index], cell_temp_array) != HAL_OK)
        {
            return HAL_ERROR;
        }
        curr_channel_read_index = (curr_channel_read_index + 1) % 14;
    }

    return HAL_OK;
}

HAL_StatusTypeDef batt_read_cell_voltages_ADSV(float *cell_voltage_array)
{
#if LTC_CHIP != ADBMS_CHIP_6830B
    (void)cell_voltage_array;
    return HAL_ERROR;
#else
    /* Mirror batt_read_cell_voltages: wake → start conversion → settle → wake → read back. */
    if (batt_spi_wakeup(false /* not sleeping*/))
    {
        return HAL_ERROR;
    }

    // Open wire switches off, so this is a plain S-ADC reading
    if (clearAndStartConversion(ADSV) != HAL_OK) {
        return HAL_ERROR;
    }

    waitForSAdcConversion();

    if (batt_spi_wakeup(false /* not sleeping*/))
    {
        return HAL_ERROR;
    }

    if (batt_read_ADSV(cell_voltage_array) != HAL_OK)
    {
        return HAL_ERROR;
    }

    return HAL_OK;
#endif
}

#if LTC_CHIP == ADBMS_CHIP_6830B
// Each chip's die temperature, see batt_get_die_temp. Information only
static float dieTempLatest[NUM_DEVICES];
static float dieTempHighest[NUM_DEVICES];
static bool dieTempRead[NUM_DEVICES];
static bool dieTempWarned[NUM_DEVICES];

// Reads every chip's die temperature (ADAX of ITMP, then status A). A failed read or a reset value just leaves the
// previous reading, and the warnings only print
static void readDieTemps(void)
{
    static uint8_t stata[NUM_BOARDS][NUM_LTC_CHIPS_PER_BOARD][STATUS_SIZE];

    if (batt_spi_wakeup(false) != HAL_OK || clearAndStartConversion(ADAX_ITMP) != HAL_OK) {
        return;
    }
    delay_us(TEMP_MEASURE_DELAY_US);
    if (batt_spi_wakeup(false) != HAL_OK || batt_read_rdstata(stata) != HAL_OK) {
        return;
    }

    for (int board = 0; board < NUM_BOARDS; board++) {
        for (int chip = 0; chip < NUM_LTC_CHIPS_PER_BOARD; chip++) {
            const int dev = board * NUM_LTC_CHIPS_PER_BOARD + chip;
            // Bytes 2-3, LSB first. 0x7FFF after power up or sleep, 0x8000 after CLRAUX: not converted
            const int16_t code = (int16_t)(((uint16_t)stata[board][chip][3] << 8) | stata[board][chip][2]);
            if (code == ADC_RESET_CODE || code == INT16_MAX) {
                continue;
            }
            // Temperature = (ITMP * 150 uV + 1.5 V) / 7.5 mV/C - 273 C = ITMP * 0.02 - 73 C (Table 105)
            const float temp = (code * 0.02f) - 73.0f;
            dieTempLatest[dev] = temp;
            if (!dieTempRead[dev] || temp > dieTempHighest[dev]) {
                dieTempHighest[dev] = temp;
            }
            dieTempRead[dev] = true;

            if (!dieTempWarned[dev] && temp >= DIE_TEMP_WARN_C) {
                ERROR_PRINT("WARN: AMS board %d chip %d die at %.1f C\n", board, chip, temp);
                dieTempWarned[dev] = true;
            } else if (dieTempWarned[dev] && temp < DIE_TEMP_WARN_CLEAR_C) {
                ERROR_PRINT("AMS board %d chip %d die back down to %.1f C\n", board, chip, temp);
                dieTempWarned[dev] = false;
            }
        }
    }
}

bool batt_get_die_temp(int dev, float *latest, float *highest)
{
    if (dev < 0 || dev >= NUM_DEVICES || !dieTempRead[dev]) {
        return false;
    }
    *latest = dieTempLatest[dev];
    *highest = dieTempHighest[dev];
    return true;
}
#endif

HAL_StatusTypeDef batt_read_cell_voltages_and_temps(float *cell_voltage_array, float *cell_temp_array){

    if (batt_read_cell_voltages(cell_voltage_array) != HAL_OK) {
        return HAL_ERROR;
    }
    if (batt_read_cell_temps(cell_temp_array) != HAL_OK) {
        ERROR_PRINT("Failed to read cell temperatures\n");
        return HAL_ERROR;
    }

#if LTC_CHIP == ADBMS_CHIP_6830B
    // After the thermistor sweep. Information only, so it never fails this read
    readDieTemps();
#endif

    return HAL_OK;
}

float float_abs(float x)
{
    if (x<0) {
        return (-1*x);
    } else {
        return x;
    }
}

void addCellVoltages(float *cell_voltages, float *cell_voltages_average)
{
    for (int i = 0; i < CELLS_PER_BOARD * NUM_BOARDS; i++) {
        cell_voltages_average[i] += cell_voltages[i];
    }
}

void divideCellVoltages(float *cell_voltages_average, unsigned int num_readings)
{
    for (int i = 0; i < CELLS_PER_BOARD * NUM_BOARDS; i++) {
        cell_voltages_average[i] /= num_readings;
    }
}

// Arrays to use with open wire test
float cell_voltages_single_reading[CELLS_PER_BOARD * NUM_BOARDS];

// The readings the open wire test compares
typedef enum {
    OPEN_WIRE_PASS_C = 0,  // C-ADC
    OPEN_WIRE_PASS_S_EVEN, // S-ADC with the open wire switches on for even channels
    OPEN_WIRE_PASS_S_ODD,  // S-ADC with the open wire switches on for odd channels
    NUM_OPEN_WIRE_PASSES
} open_wire_pass_t;

#if LTC_CHIP == ADBMS_CHIP_6830B
static const ltc_command_t OPEN_WIRE_PASS_COMMANDS[NUM_OPEN_WIRE_PASSES] = { ADCV, ADSV_OW_EVEN, ADSV_OW_ODD };
#endif

// Waits for an open wire pass's conversion: about 1 ms for the C-ADC, 8 ms for the S-ADC
static void waitForOpenWirePassConversion(open_wire_pass_t pass)
{
    if (pass == OPEN_WIRE_PASS_C) {
        vTaskDelay(VOLTAGE_MEASURE_DELAY_MS);
        delay_us(VOLTAGE_MEASURE_DELAY_EXTRA_US);
    } else {
        waitForSAdcConversion();
    }
}

// Perform open wire test cell voltage reading for one pass
// Sums num_readings voltages into cell_voltages to account for noise; the caller divides.
HAL_StatusTypeDef performOpenCircuitTestReading(float *cell_voltages, open_wire_pass_t pass,
                                                unsigned int num_readings)
{
    if (num_readings <= 0) {
        return HAL_ERROR;
    }

    for (int i = 0; i < num_readings; i++) {
        if (batt_spi_wakeup(false /* not sleeping*/)) {
            return HAL_ERROR;
        }

        #if LTC_CHIP == LTC_CHIP_6804 || LTC_CHIP == LTC_CHIP_6812
        if (batt_broadcast_command(pass == OPEN_WIRE_PASS_C ? ADOW_UP : ADOW_DOWN) != HAL_OK) {
            return HAL_ERROR;
        }
        #elif LTC_CHIP == ADBMS_CHIP_6830B
        if (clearAndStartConversion(OPEN_WIRE_PASS_COMMANDS[pass]) != HAL_OK) {
            return HAL_ERROR;
        }
        #endif

        waitForOpenWirePassConversion(pass);

		if (batt_spi_wakeup(false /* not sleeping*/))
		{
			return HAL_ERROR;
		}

        #if LTC_CHIP == ADBMS_CHIP_6830B
        if (pass != OPEN_WIRE_PASS_C) {
            if (batt_read_ADSV(cell_voltages_single_reading) != HAL_OK) {
                return HAL_ERROR;
            }
        } else
        #endif
        {
            if (batt_readBackCellVoltage(cell_voltages_single_reading, OPEN_WIRE) != HAL_OK) {
                return HAL_ERROR;
            }
        }

		addCellVoltages(cell_voltages_single_reading, cell_voltages);
    }

    return HAL_OK;
}

// Cells with bleed resistors removed. 0-based, so battInfo's cell number minus 1
// S-ADC measures through the discharge path so the
// ADCV/ADSV ratio check will not work on these cells, and they can't be balanced
// Empty since the rev C AMS boards: every cell has its bleed resistor
static const uint8_t NO_DISCHARGE_CELLS[] = {};

bool batt_cell_can_discharge(int cell) {
    if (!NO_DISCHARGE_CELLS_ENABLED) {
        return true;
    }
    for (size_t i = 0; i < sizeof(NO_DISCHARGE_CELLS) / sizeof(NO_DISCHARGE_CELLS[0]); i++) {
        if (NO_DISCHARGE_CELLS[i] == cell) {
            return false;
        }
    }
    return true;
}

// Cells on either side of a high-resistance sense tap. 0-based, so battInfo's cell number minus 1
// Draining them displaces the tap for longer than the balance pause, so never balance them
// Empty since the rev C AMS boards
static const uint8_t DO_NOT_BALANCE_CELLS[] = {};

bool batt_cell_can_balance(int cell) {
    if (!batt_cell_can_discharge(cell)) {
        return false;
    }
#if LTC_CHIP == ADBMS_CHIP_6830B
    if (isBalanceHeldOff(cell / CELLS_PER_BOARD, (cell % CELLS_PER_BOARD) / CELLS_PER_CHIP)) {
        return false;
    }
#endif
    if (!DO_NOT_BALANCE_CELLS_ENABLED) {
        return true;
    }
    for (size_t i = 0; i < sizeof(DO_NOT_BALANCE_CELLS) / sizeof(DO_NOT_BALANCE_CELLS[0]); i++) {
        if (DO_NOT_BALANCE_CELLS[i] == cell) {
            return false;
        }
    }
    return true;
}

HAL_StatusTypeDef checkForOpenCircuit(bool *readFailed)
{
    *readFailed = false;

    // Perform averaging of multiple voltage readings to account for potential
    // bad connections to AMS boards that causes noise. Static, since three of these don't fit the task's stack
    static float cell_voltages_pass[NUM_OPEN_WIRE_PASSES][CELLS_PER_BOARD * NUM_BOARDS];
    memset(cell_voltages_pass, 0, sizeof(cell_voltages_pass));

	float last_IBus = 0.0f;
	// If we can't get it from the IBus queue, skip the check
	bool skip_IBus_check = (getIBus(&last_IBus) != HAL_OK);

    for (int pass = 0; pass < NUM_OPEN_WIRE_PASSES; pass++) {
        if (performOpenCircuitTestReading(cell_voltages_pass[pass], (open_wire_pass_t)pass,
                                          NUM_OPEN_WIRE_TEST_VOLTAGE_READINGS)
            != HAL_OK)
        {
            *readFailed = true;
            return HAL_ERROR;
        }
        divideCellVoltages(cell_voltages_pass[pass], NUM_OPEN_WIRE_TEST_VOLTAGE_READINGS);
    }

    float curr_IBus = 0.0f;
	skip_IBus_check |= (getIBus(&curr_IBus) != HAL_OK);
	if (!skip_IBus_check)
	{
		float IBus_error = float_abs(last_IBus - curr_IBus);
		if (IBus_error > OPEN_WIRE_IBUS_TOLERANCE_A)
		{
			// Open Wire check only really works if IBus is constant
			DEBUG_PRINT("Sharp IBus spike over > %f A, at %f A\n", OPEN_WIRE_IBUS_TOLERANCE_A, IBus_error);
			return HAL_OK;
		}
	}
    HAL_StatusTypeDef ret = HAL_OK;

    for (int cell = 0; cell < NUM_VOLTAGE_CELLS; cell++)
    {
        const float c = cell_voltages_pass[OPEN_WIRE_PASS_C][cell];
        const float sEvenOn = cell_voltages_pass[OPEN_WIRE_PASS_S_EVEN][cell];
        const float sOddOn = cell_voltages_pass[OPEN_WIRE_PASS_S_ODD][cell];
        const char *reason = batt_open_wire_reason(cell, c, sEvenOn, sOddOn);
        if (reason != NULL) {
            ERROR_PRINT("Cell %d open: %s (C %.4f, S even %.4f, S odd %.4f)\n", cell, reason, c, sEvenOn, sOddOn);
            ret = HAL_ERROR;
        }
    }

    return ret;
}

/*
 * Same per cell criteria as checkForOpenCircuit. Returns NULL if the cell looks connected, otherwise why it looks open.
 * Cells in NO_DISCHARGE_CELLS can't be judged by their S-ADC readings, so they only get the C-ADC check
 */
const char *batt_open_wire_reason(int cell, float c, float sEvenOn, float sOddOn)
{
    if (float_abs(c) < OPEN_WIRE_MIN_DIVISOR_V) {
        return "C at 0 V";
    }
    if (!batt_cell_can_discharge(cell)) {
        return NULL;
    }

    // The datasheet numbers each chip's channels from 1, so the even channels are the odd 0-based cells
    const bool evenChannel = ((cell % CELLS_PER_CHIP) % 2) == 1;
    // No abs(): a negative ratio is a wiring fault too
    const float ratioSwitchOn = (evenChannel ? sEvenOn : sOddOn) / c;
    const float ratioSwitchOff = (evenChannel ? sOddOn : sEvenOn) / c;
    if (ratioSwitchOn < OPEN_WIRE_SWITCH_ON_RATIO_MIN || ratioSwitchOn > OPEN_WIRE_SWITCH_ON_RATIO_MAX) {
        return "switch-on S/C out of range";
    }
    if (ratioSwitchOff < OPEN_WIRE_SWITCH_OFF_RATIO_MIN || ratioSwitchOff > OPEN_WIRE_SWITCH_OFF_RATIO_MAX) {
        return "switch-off S/C out of range";
    }
    return NULL;
}

#if LTC_CHIP == ADBMS_CHIP_6830B
/*
 * Open wire readings that don't need every configured board to answer, e.g. one segment on the bench with
 * NUM_SEGMENTS 5. Averages the C-ADC and both S-ADC open wire passes like checkForOpenCircuit, and deviceOk[dev] is
 * false for any device that didn't answer. Judge the readings with batt_open_wire_reason
 */
HAL_StatusTypeDef batt_open_wire_scan(bool deviceOk[NUM_DEVICES], float *c, float *sEvenOn, float *sOddOn)
{
    // Own buffer, so this doesn't share cell_voltages_single_reading with the battery task
    static float singleReading[NUM_VOLTAGE_CELLS];
    float *sums[NUM_OPEN_WIRE_PASSES] = { c, sEvenOn, sOddOn };

    for (int pass = 0; pass < NUM_OPEN_WIRE_PASSES; pass++) {
        memset(sums[pass], 0, sizeof(float) * NUM_VOLTAGE_CELLS);
    }
    for (int dev = 0; dev < NUM_DEVICES; dev++) {
        deviceOk[dev] = true;
    }

    // The chips may have been asleep and lost their config and reference, e.g. once the battery task stops on a fault
    if (batt_spi_wakeup(true) != HAL_OK || batt_write_config() != HAL_OK) {
        return HAL_ERROR;
    }
    vTaskDelay(pdMS_TO_TICKS(5)); // Reference power up

    for (int pass = 0; pass < NUM_OPEN_WIRE_PASSES; pass++) {
        const bool cAdc = (pass == OPEN_WIRE_PASS_C);
        for (int reading = 0; reading < NUM_OPEN_WIRE_TEST_VOLTAGE_READINGS; reading++) {
            if (batt_spi_wakeup(false) != HAL_OK || clearAndStartConversion(OPEN_WIRE_PASS_COMMANDS[pass]) != HAL_OK) {
                return HAL_ERROR;
            }
            waitForOpenWirePassConversion((open_wire_pass_t)pass);

            bool readOk[NUM_DEVICES];
            memset(singleReading, 0, sizeof(singleReading));
            if (batt_read_cell_results_partial(cAdc, singleReading, readOk) != HAL_OK) {
                return HAL_ERROR;
            }
            for (int dev = 0; dev < NUM_DEVICES; dev++) {
                deviceOk[dev] = deviceOk[dev] && readOk[dev];
            }
            addCellVoltages(singleReading, sums[pass]);
        }
        divideCellVoltages(sums[pass], NUM_OPEN_WIRE_TEST_VOLTAGE_READINGS);
    }

    return HAL_OK;
}

/*
 * The chips set these flags by themselves, and they stay set until cleared:
 * - CSxFLT (status C): the redundant ADCV (RD = 1) in batt_read_cell_voltages found a cell's C-ADC and S-ADC results
 *   differing by more than CTH
 * - CxOV/CxUV (status D): a C-ADC conversion was beyond VOV/VUV, a backstop to the software limits
 * Both are read here and then cleared. Every flag of one chip is printed on one line, bit n being the chip's cell n
 */
HAL_StatusTypeDef batt_check_diagnostic_flags(bool *readFailed)
{
    static uint8_t statc[NUM_BOARDS][NUM_LTC_CHIPS_PER_BOARD][STATUS_SIZE];
    static uint8_t statd[NUM_BOARDS][NUM_LTC_CHIPS_PER_BOARD][STATUS_SIZE];
    *readFailed = false;

    if (batt_spi_wakeup(false) != HAL_OK || batt_read_rdstatc(statc) != HAL_OK || batt_read_rdstatd(statd) != HAL_OK) {
        *readFailed = true;
        return HAL_ERROR;
    }

    HAL_StatusTypeDef ret = HAL_OK;
    for (int board = 0; board < NUM_BOARDS; board++) {
        for (int chip = 0; chip < NUM_LTC_CHIPS_PER_BOARD; chip++) {
            const int firstCell = board * CELLS_PER_BOARD + chip * CELLS_PER_CHIP;
            uint16_t csMismatch = 0;
            uint16_t overvoltage = 0;
            uint16_t undervoltage = 0;

            // Reset since batt_recover_reset_chips last ran: its flags are back at their defaults (all set), so they
            // mean nothing. The next pass restores it
            if (GETBIT(statc[board][chip][RDSTATC_STER5_IDX], RDSTATC_SLEEP_BIT)) {
                continue;
            }

            // Only the wired cells. Each status D byte holds 4 cells, UV in the even bit and OV in the odd bit
            for (int c = 0; c < CELLS_PER_CHIP; c++) {
                // S-ADC readings of a cell without its discharge path aren't comparable
                if (GETBIT(statc[board][chip][c / 8], c % 8) && batt_cell_can_discharge(firstCell + c)) {
                    csMismatch |= (1u << c);
                }
                if (GETBIT(statd[board][chip][c / 4], (c % 4) * 2 + 1)) {
                    overvoltage |= (1u << c);
                }
                if (GETBIT(statd[board][chip][c / 4], (c % 4) * 2)) {
                    undervoltage |= (1u << c);
                }
            }

            if (csMismatch || overvoltage || undervoltage) {
                ERROR_PRINT("AMS board %d chip %d (cells %d-%d): C/S mismatch 0x%04X, HW OV 0x%04X, HW UV 0x%04X\n",
                            board, chip, firstCell, firstCell + CELLS_PER_CHIP - 1,
                            csMismatch, overvoltage, undervoltage);
            }
            // Only the kinds of flag set to fault, the rest are just the print above
            if ((CS_MISMATCH_FAULT_ENABLED && csMismatch) || (HW_OV_FAULT_ENABLED && overvoltage)
                || (HW_UV_FAULT_ENABLED && undervoltage)) {
                ret = HAL_ERROR;
            }
        }
    }

    if (batt_clear_cs_flags() != HAL_OK || batt_clear_ovuv_flags() != HAL_OK) {
        // A flag that was set is still reported as one
        if (ret == HAL_OK) {
            *readFailed = true;
        }
        return HAL_ERROR;
    }
    return ret;
}

/// Times each chip has reset since boot, for the prints
static uint32_t chipResetCount[NUM_DEVICES];
/// Set when a chip's reset is reported, cleared once its SLEEP flag reads clear, so a reset whose restore fails or
/// doesn't take is reported once and not again every pass while it's retried
static bool chipResetReported[NUM_DEVICES];

/*
 * A chip that power cycled or slept (a brown-out from a connector bounce, a thermal shutdown, or no valid command for
 * about 2 s) comes back with every register at its default: reference off, CTH, VOV/VUV, MUX and discharge settings
 * gone, and all its flags set. Its SLEEP flag says so. Writing the configuration and PWM registers again restores it,
 * every chip getting its current settings, and its flags are cleared once the reference is back up. That also clears
 * flags the other chips set since the last flag check, but only on a pass where a chip reset.
 * A chip that shut down from heat gets its balancing turned off first and held off for
 * THERMAL_SHUTDOWN_BALANCE_HOLD_OFF_MS, so restoring it doesn't heat it straight into another shutdown.
 * resets[dev] is only set the pass a reset is first seen
 */
HAL_StatusTypeDef batt_recover_reset_chips(chip_reset_t resets[NUM_DEVICES])
{
    static uint8_t statc[NUM_BOARDS][NUM_LTC_CHIPS_PER_BOARD][STATUS_SIZE];
    for (int dev = 0; dev < NUM_DEVICES; dev++) {
        resets[dev] = CHIP_NOT_RESET;
    }
    if (batt_spi_wakeup(false) != HAL_OK || batt_read_rdstatc(statc) != HAL_OK) {
        return HAL_ERROR;
    }

    bool anyReset = false;
    for (int board = 0; board < NUM_BOARDS; board++) {
        for (int chip = 0; chip < NUM_LTC_CHIPS_PER_BOARD; chip++) {
            const uint8_t flags = statc[board][chip][RDSTATC_STER5_IDX];
            const int dev = board * NUM_LTC_CHIPS_PER_BOARD + chip;
            if (!GETBIT(flags, RDSTATC_SLEEP_BIT)) {
                chipResetReported[dev] = false;
                continue;
            }
            // Restore it again whatever happens, but report the reset only once
            anyReset = true;
            if (chipResetReported[dev]) {
                continue;
            }
            chipResetReported[dev] = true;
            chipResetCount[dev]++;

            if (GETBIT(flags, RDSTATC_THERMAL_SHUTDOWN_BIT)) {
                resets[dev] = CHIP_THERMAL_SHUTDOWN;
                balanceHeldOff[dev] = true;
                balanceHeldOffTick[dev] = xTaskGetTickCount();
                for (int cell = 0; cell < CELLS_PER_CHIP; cell++) {
                    batt_unset_balancing_cell(board, chip, cell);
                }
                ERROR_PRINT("WARN: AMS board %d chip %d thermal shutdown, restoring it, no balancing for %lu s\n",
                            board, chip, THERMAL_SHUTDOWN_BALANCE_HOLD_OFF_MS / 1000UL);
            } else {
                resets[dev] = CHIP_RESET;
                ERROR_PRINT("WARN: AMS board %d chip %d power cycled or slept, restoring it\n", board, chip);
            }
            ERROR_PRINT("AMS board %d chip %d: %lu resets since boot\n", board, chip,
                        (unsigned long)chipResetCount[dev]);
        }
    }
    if (!anyReset) {
        return HAL_OK;
    }

    if (batt_write_config() != HAL_OK || batt_write_config_pwm() != HAL_OK) {
        return HAL_ERROR;
    }
    // The clears only work once the reference is back up
    vTaskDelay(pdMS_TO_TICKS(T_REFUP_MS) + 1);
    if (batt_spi_wakeup(false) != HAL_OK || batt_clear_cs_flags() != HAL_OK || batt_clear_ovuv_flags() != HAL_OK
        || batt_clear_reset_flags() != HAL_OK) {
        return HAL_ERROR;
    }
    return HAL_OK;
}

bool batt_get_cs_diff(int cell, float *latest, float *lowest, float *highest)
{
#if CS_DIFF_LOG_ENABLED
    if (cell < 0 || cell >= NUM_VOLTAGE_CELLS || csDiffSamples == 0) {
        return false;
    }
    *latest = csDiffLatest[cell];
    *lowest = csDiffLowest[cell];
    *highest = csDiffHighest[cell];
    return true;
#else
    (void)cell;
    (void)latest;
    (void)lowest;
    (void)highest;
    return false;
#endif
}

uint32_t batt_get_cs_diff_samples(int *worstCell)
{
    *worstCell = -1;
#if CS_DIFF_LOG_ENABLED
    float worst = 0.0f;
    for (int cell = 0; csDiffSamples > 0 && cell < NUM_VOLTAGE_CELLS; cell++) {
        const float largest = fmaxf(float_abs(csDiffLowest[cell]), float_abs(csDiffHighest[cell]));
        if (*worstCell < 0 || largest > worst) {
            worst = largest;
            *worstCell = cell;
        }
    }
    return csDiffSamples;
#else
    return 0;
#endif
}

void batt_reset_cs_diff(void)
{
#if CS_DIFF_LOG_ENABLED
    csDiffSamples = 0;
#endif
}
#endif

// Need to write config after
HAL_StatusTypeDef batt_balance_cell(int cell)
{
    if (c_assert(cell < NUM_VOLTAGE_CELLS))
    {
        DEBUG_PRINT("Tried to balance a cell out of range (needs to be < %u)\r\n", NUM_VOLTAGE_CELLS);
        return HAL_ERROR;
    }

    int boardIdx = cell / CELLS_PER_BOARD;
    int chipIdx = (cell % CELLS_PER_BOARD) / CELLS_PER_CHIP;
    int amsCellIdx = cell % CELLS_PER_CHIP;

#if LTC_CHIP == ADBMS_CHIP_6830B
    // Covers every way of turning balancing on, resumeBalance and the CLI included
    if (isBalanceHeldOff(boardIdx, chipIdx)) {
        return HAL_OK;
    }
#endif

    batt_set_balancing_cell(boardIdx, chipIdx, amsCellIdx, BALANCE_PWM_DUTY_MAX);

    return HAL_OK;
}

HAL_StatusTypeDef batt_stop_balance_cell(int cell)
{
    if (c_assert(cell < NUM_VOLTAGE_CELLS))
    {
        return HAL_ERROR;
    }

    int boardIdx = cell / CELLS_PER_BOARD;
    int chipIdx = (cell % CELLS_PER_BOARD) / CELLS_PER_CHIP;
    int amsCellIdx = cell % CELLS_PER_CHIP;

    batt_unset_balancing_cell(boardIdx, chipIdx, amsCellIdx);

    return HAL_OK;
}

// Need to read config first
bool batt_is_cell_balancing(int cell)
{
    if (c_assert(cell < NUM_VOLTAGE_CELLS))
    {
        return false;
    }

    int boardIdx = cell / CELLS_PER_BOARD;
    int chipIdx = (cell % CELLS_PER_BOARD) / CELLS_PER_CHIP;
    int amsCellIdx = cell - (boardIdx * CELLS_PER_BOARD + chipIdx * CELLS_PER_CHIP);

    return batt_get_balancing_cell_state(boardIdx, chipIdx, amsCellIdx);
}

HAL_StatusTypeDef batt_unset_balancing_all_cells(void)
{
    for (int board = 0; board < NUM_BOARDS; board++) {
    	for(int chip = 0; chip < NUM_LTC_CHIPS_PER_BOARD; chip++) {
			for (int cell = 0; cell < CELLS_PER_CHIP; cell++) {
				batt_unset_balancing_cell(board, chip, cell);
			}
		}
    }

    return HAL_OK;
}

HAL_StatusTypeDef batt_write_balancing_config(void)
{
    if (batt_write_config_pwm() != HAL_OK) {
        ERROR_PRINT("batt_write_config_pwm: WRPWM A/B failed\n");
        return HAL_ERROR;
    }

    if (batt_write_config() != HAL_OK) {
        ERROR_PRINT("batt_write_config: WRCFGA/B failed\n");
        return HAL_ERROR;
    }

    return HAL_OK;
}

HAL_StatusTypeDef batt_set_disharge_timer(DischargeTimerLength length)
{
    if (length >= INVALID_DT_TIME) {
        return HAL_ERROR;
    }
	
    return batt_config_discharge_timer(length);

}

HAL_StatusTypeDef balanceTest()
{
    if (batt_spi_wakeup(true) != HAL_OK)
    {
        return HAL_ERROR;
    }

    batt_init_chip_configs();
    batt_init_chip_configs_pwm();
    if (batt_balance_cell(0) != HAL_OK) {
        return HAL_ERROR;
    }

    batt_set_disharge_timer(DT_30_SEC);

    if (batt_write_balancing_config() != HAL_OK)
    {
        return HAL_ERROR;
    }

    vTaskDelay(40000);
    if (batt_unset_balancing_all_cells() != HAL_OK) {
        return HAL_ERROR;
    }

    if (batt_spi_wakeup(true) != HAL_OK)
    {
        return HAL_ERROR;
    }
    if (batt_write_balancing_config() != HAL_OK)
    {
        return HAL_ERROR;
    }

    return HAL_OK;
}


