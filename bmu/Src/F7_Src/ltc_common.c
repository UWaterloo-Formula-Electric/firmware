#include <string.h>

#include "ltc_common.h"
#include "ltc_chip.h"
#include "math.h"

// Write a broadcast command and pec to a tx buffer
// Make sure txBuffer is big enough
HAL_StatusTypeDef batt_format_command(uint8_t cmdByteLow, uint8_t cmdByteHigh, uint8_t *txBuffer)
{
    txBuffer[0] = cmdByteLow;
    txBuffer[1] = cmdByteHigh;
    batt_gen_pec(txBuffer, COMMAND_SIZE, &(txBuffer[COMMAND_SIZE]));
    return HAL_OK;
}

HAL_StatusTypeDef batt_format_write_config_command(uint8_t cmdByteLow, uint8_t cmdByteHigh, uint8_t *txBuffer, uint8_t writeData[NUM_BOARDS][NUM_LTC_CHIPS_PER_BOARD][BATT_CONFIG_SIZE], uint8_t writeDataSize)
{
    uint8_t data_PEC[2];
    uint8_t txBufferIndex = COMMAND_SIZE + PEC_SIZE;

    batt_format_command(cmdByteLow, cmdByteHigh, txBuffer);

    for (int board = NUM_BOARDS - 1; board >= 0; --board)
    {
        for (int chip = NUM_LTC_CHIPS_PER_BOARD - 1; chip >= 0; --chip)
        {
            #if LTC_CHIP == ADBMS_CHIP_6830B
                batt_gen_pec_data((uint8_t*) &(writeData[board][chip]), writeDataSize, data_PEC, 0);
            #else
                batt_gen_pec((uint8_t*) &(writeData[board][chip]), writeDataSize, data_PEC);
            #endif
            memcpy(&txBuffer[txBufferIndex], (uint8_t*) &(writeData[board][chip]), writeDataSize);
            txBufferIndex += writeDataSize;
            memcpy(&txBuffer[txBufferIndex], data_PEC, PEC_SIZE);
            txBufferIndex += PEC_SIZE;
        }
    }
    return HAL_OK;
}

/*
 * Generates a 15bit PEC for the message defined for data.
 * Uses CRC with polynomial:
 *   x^15 + x^14 + x^10 + x^8 + x^7 + x^4 + x^3 + 1
 *
 * NOTE: Wondering how this works? Beats me. Check out the LTC6804 datasheet.
 */
void batt_gen_pec(uint8_t * arrdata, unsigned int num_bytes, uint8_t * pecAddr) {
    unsigned char in0;
    unsigned char in3;
    unsigned char in4;
    unsigned char in7;
    unsigned char in8;
    unsigned char in10;
    unsigned char in14;
    int i;
    int n;

    unsigned short pec = PEC_INIT_VAL;
    for (n = 0; n < num_bytes; n++) {
        uint8_t data = arrdata[n];
        for (i = 0; i < 8; i++, data = data << 1) {
            unsigned char din = GETBIT(data, 7);

            in0 = din ^ GETBIT(pec, 14);
            in3 = in0 ^ GETBIT(pec, 2);
            in4 = in0 ^ GETBIT(pec, 3);
            in7 = in0 ^ GETBIT(pec, 6);
            in8 = in0 ^ GETBIT(pec, 7);
            in10 = in0 ^ GETBIT(pec, 9);
            in14 = in0 ^ GETBIT(pec, 13);

            ASSIGNBIT(pec, in14, 14);
            ASSIGNBIT(pec, GETBIT(pec, 12), 13);
            ASSIGNBIT(pec, GETBIT(pec, 11), 12);
            ASSIGNBIT(pec, GETBIT(pec, 10), 11);
            ASSIGNBIT(pec, in10, 10);
            ASSIGNBIT(pec, GETBIT(pec,8), 9);
            ASSIGNBIT(pec, in8, 8);
            ASSIGNBIT(pec, in7, 7);
            ASSIGNBIT(pec, GETBIT(pec, 5), 6);
            ASSIGNBIT(pec, GETBIT(pec, 4), 5);
            ASSIGNBIT(pec, in4, 4);
            ASSIGNBIT(pec, in3, 3);
            ASSIGNBIT(pec, GETBIT(pec, 1), 2);
            ASSIGNBIT(pec, GETBIT(pec, 0), 1);
            ASSIGNBIT(pec, in0, 0);
        }
    }

    // Stuff the LSB with a 0, since pec are only 15 bits but we have 2 bytes to send
    pec = (pec << 1);
    CLEARBIT(pec, 0);

    pecAddr[0] = (pec >> 8) & 0xff;
    pecAddr[1] = pec & 0xff;
}

/*
 * Generates a 10-bit PEC for the message defined for data.
 * Uses CRC with polynomial:
 *   x10 + x7 + x3 + x2 + x + 1
 *
 * NOTE: Only used for the ADBMS6830, which requires a different PEC for the data
 */
void batt_gen_pec_data(uint8_t * arrdata, unsigned int num_bytes, uint8_t * pecAddr, uint8_t cmd_counter) {
    unsigned char in0, in1, in2, in3, in7;
    int i, n;

    // Initial value for Data PEC is 0x0010 (10-bit)
    unsigned short pec = 0x0010; 

    for (n = 0; n < num_bytes; n++) {
        uint8_t data = arrdata[n];
        for (i = 0; i < 8; i++, data = data << 1) {
            unsigned char din = (data >> 7) & 0x01;
            
            // Logic derived from polynomial: x10 + x7 + x3 + x2 + x + 1
            // Feedback bit is (din XOR pec[9])
            unsigned char fb = din ^ ((pec >> 9) & 0x01);

            in0 = fb;
            in1 = fb ^ ((pec >> 0) & 0x01);
            in2 = fb ^ ((pec >> 1) & 0x01);
            in3 = fb ^ ((pec >> 2) & 0x01);
            in7 = fb ^ ((pec >> 6) & 0x01);

            // Shift and update bits
            pec = (pec << 1) & 0x3FF; // Keep it 10-bit
            
            // Assign specific bits based on polynomial taps
            if (in0) pec |= (1 << 0); else pec &= ~(1 << 0);
            if (in1) pec |= (1 << 1); else pec &= ~(1 << 1);
            if (in2) pec |= (1 << 2); else pec &= ~(1 << 2);
            if (in3) pec |= (1 << 3); else pec &= ~(1 << 3);
            if (in7) pec |= (1 << 7); else pec &= ~(1 << 7);
        }
    }

    // Process the 6-bit Command Counter or padding 0s
    for (i = 0; i < 6; i++) {
        unsigned char din = (cmd_counter >> (5 - i)) & 0x01;
        unsigned char fb = din ^ ((pec >> 9) & 0x01);

        in0 = fb;
        in1 = fb ^ ((pec >> 0) & 0x01);
        in2 = fb ^ ((pec >> 1) & 0x01);
        in3 = fb ^ ((pec >> 2) & 0x01);
        in7 = fb ^ ((pec >> 6) & 0x01);

        pec = (pec << 1) & 0x3FF;
        
        if (in0) pec |= (1 << 0); else pec &= ~(1 << 0);
        if (in1) pec |= (1 << 1); else pec &= ~(1 << 1);
        if (in2) pec |= (1 << 2); else pec &= ~(1 << 2);
        if (in3) pec |= (1 << 3); else pec &= ~(1 << 3);
        if (in7) pec |= (1 << 7); else pec &= ~(1 << 7);
    }

    // Format for Output: [PEC0] [PEC1]
    // PEC0 bits 7-2 are Cmd Counter, bits 1-0 are PEC[9:8]
    // PEC1 bits 7-0 are PEC[7:0]
    pecAddr[0] = ((cmd_counter & 0x3F) << 2) | ((pec >> 8) & 0x03); 
    pecAddr[1] = pec & 0xFF;
}

/*
 * Check the PEC on received data
 * @param rxBuffer: buffer holding the read data and PEC
 * @param dataSize: length of the data the PEC is calculated on
 */
HAL_StatusTypeDef checkPEC(uint8_t *rxBuffer, size_t dataSize)
{
    uint8_t pec[2];
    batt_gen_pec(rxBuffer, dataSize, pec);
    uint32_t pec_index = dataSize;
    
    if (pec[0] == rxBuffer[pec_index] && pec[1] == rxBuffer[pec_index + 1])
    {
        return HAL_OK;
    } else {
        if (PRINT_ALL_PEC_ERRORS) {
            DEBUG_PRINT("%u != %u. %u != %u\r\n", pec[0],  rxBuffer[pec_index], pec[1], rxBuffer[pec_index + 1]);
        }
        return HAL_ERROR;
    }
}

/*
 * Check the PEC on received data
 * @param rxBuffer: buffer holding the read data and PEC
 * @param dataSize: length of the data the PEC is calculated on
 */
HAL_StatusTypeDef checkPECData(uint8_t *rxBuffer, size_t dataSize)
{
    uint8_t pec[2];
    
    uint8_t cmd_counter = (rxBuffer[dataSize] >> 2) & 0x3F;
    batt_gen_pec_data(rxBuffer, dataSize, pec, cmd_counter);
    
    uint32_t pec_index = dataSize;
    if (pec[0] == rxBuffer[pec_index] && pec[1] == rxBuffer[pec_index + 1])
    {
        return HAL_OK;
    } else {
        // The caller prints which device failed
        return HAL_ERROR;
    }
}

void fillDummyBytes(uint8_t * buf, uint32_t length)
{
    for (uint32_t dbyte = 0; dbyte < length; dbyte++)
    {
        buf[dbyte] = 0xFF;
    }
}

// When the chain last saw any traffic (commands and wake pulses), and when it last got a command, so batt_spi_wakeup
// knows how far it may have powered down. chainCommanded stays false until the first command
static TickType_t lastTrafficTick = 0;
static TickType_t lastCommandTick = 0;
static bool chainCommanded = false;

static void recordCommandSent(void)
{
    lastCommandTick = xTaskGetTickCount();
    lastTrafficTick = lastCommandTick;
    chainCommanded = true;
}

/* HSPI send/receive function */
#define HSPI_TIMEOUT 15
HAL_StatusTypeDef spi_tx_rx(uint8_t * tdata, uint8_t * rbuffer,
                            unsigned int len) {
    //Make sure rbuffer is large enough for the sending command + receiving bytes
    HAL_GPIO_WritePin(ISO_SPI_NSS_GPIO_Port, ISO_SPI_NSS_Pin, GPIO_PIN_RESET);
    HAL_StatusTypeDef status = HAL_SPI_TransmitReceive(&ISO_SPI_HANDLE, tdata, rbuffer, len, HSPI_TIMEOUT);
    delay_us(2); // t6 = (70ns * numDevices) + 950ns
    HAL_GPIO_WritePin(ISO_SPI_NSS_GPIO_Port, ISO_SPI_NSS_Pin, GPIO_PIN_SET);
    delay_us(2); // t5 = (70ns * numDevices) + 900ns
    recordCommandSent();
    return status;
}

HAL_StatusTypeDef batt_spi_tx(uint8_t *txBuffer, size_t len)
{
    HAL_GPIO_WritePin(ISO_SPI_NSS_GPIO_Port, ISO_SPI_NSS_Pin, GPIO_PIN_RESET);
    HAL_StatusTypeDef status = HAL_SPI_Transmit(&ISO_SPI_HANDLE, txBuffer, len, HSPI_TIMEOUT);
    delay_us(2); // t6 = (70ns * numDevices) + 950ns
    HAL_GPIO_WritePin(ISO_SPI_NSS_GPIO_Port, ISO_SPI_NSS_Pin, GPIO_PIN_SET);
    delay_us(2); // t5 = (70ns * numDevices) + 900ns
    recordCommandSent();
    return status;
}

/*
 * Wake the chain as far as it may have powered down. Every chip needs its own pair of long isoSPI pulses (CS low, then
 * high): a chip wakes on one pair and only passes on the next, so a single pulse wakes just the first chip (p51)
 * - Traffic under tIDLE ago: the isoSPI ports are still up, nothing to do
 * - A command under tSLEEP ago: the chips are awake but their ports have idled, so space the pulses tREADY apart
 * - Otherwise, before the first command, or with sleeping set: the chips may be asleep, so space them tWAKE apart
 * @param sleeping: Set to true to wake the chain from sleep whatever the timing says, e.g. at start up or to retry a
 * failed read
 */
int batt_spi_wakeup(bool sleeping)
{
    const TickType_t now = xTaskGetTickCount();
    const bool mayBeAsleep = sleeping || !chainCommanded || (now - lastCommandTick) > CHIPS_AWAKE_MAX_TICKS;
    if (!mayBeAsleep && (now - lastTrafficTick) <= PORT_READY_MAX_TICKS) {
        return 0;
    }

    const uint16_t spacingUs = mayBeAsleep ? WAKE_SPACING_SLEEP_US : WAKE_SPACING_STANDBY_US;
    // One pulse pair more than there are chips, in case the first only brings up the converter's own isoSPI port
    for (int pulse = 0; pulse < NUM_DEVICES + 1; pulse++) {
        HAL_GPIO_WritePin(ISO_SPI_NSS_GPIO_Port, ISO_SPI_NSS_Pin, GPIO_PIN_RESET);
        delay_us(WAKE_PULSE_LOW_US);
        HAL_GPIO_WritePin(ISO_SPI_NSS_GPIO_Port, ISO_SPI_NSS_Pin, GPIO_PIN_SET);
        delay_us(spacingUs);
    }
    lastTrafficTick = xTaskGetTickCount();

    return 0;
}

// input voltage is in Volts, precision is in increments of 100uV
// output temp is in degrees C
float batt_convert_voltage_to_temp(float voltage) {
    // for NTCAFLEX15103HH. https://www.vishay.com/docs/29132/ntcaflex05.pdf.

    const float p1 = 0.0;
    const float p2 = 0.0;
    const float p3 = 0.0;
    const float p4 = 0.0;
    const float p5 = 1.943105255511;
    const float p6 = 13.871206641654;
    const float p7 = -16.994487328784;

    float x = voltage*4.64/3.0;

    float output = p1*pow(x,6) + p2*pow(x,5) + p3*pow(x,4) + p4*pow(x,3) + p5*pow(x,2)
        + p6*pow(x,1) + p7;

    return output;
}

static uint8_t s_mock_all_thermistors;
static float s_mock_all_thermistors_C;

float batt_thermistor_adc_to_temp(int tempIdx, float voltageThermistor)
{
    (void)tempIdx;
    if (s_mock_all_thermistors) {
        return s_mock_all_thermistors_C;
    }
    return batt_convert_voltage_to_temp(voltageThermistor);
}

void batt_set_mock_all_thermistors(float temp_C)
{
    s_mock_all_thermistors = 1u;
    s_mock_all_thermistors_C = temp_C;
}

void batt_clear_mock_all_thermistors(void)
{
    s_mock_all_thermistors = 0u;
}

/* delay function for wakeup. Use for delays < 1ms to reduce tight polling time */
void delay_us(uint16_t time_us)
{
	__HAL_TIM_SetCounter(&DELAY_TIMER,0);
	__HAL_TIM_SetAutoreload(&DELAY_TIMER,0xffff);
	HAL_TIM_Base_Start(&DELAY_TIMER);
	while(DELAY_TIMER_INSTANCE->CNT < time_us);
	HAL_TIM_Base_Stop(&DELAY_TIMER);
}

void long_delay_us(const uint32_t time_us)
{
    const uint32_t ms_delay = (time_us / 1000);
    vTaskDelay(ms_delay);

    const uint32_t us_delay = (time_us % 1000);
	__HAL_TIM_SetCounter(&DELAY_TIMER,0);
	__HAL_TIM_SetAutoreload(&DELAY_TIMER,0xffff);
	HAL_TIM_Base_Start(&DELAY_TIMER);
	while(DELAY_TIMER_INSTANCE->CNT < (us_delay % 1000));
	HAL_TIM_Base_Stop(&DELAY_TIMER);
}
