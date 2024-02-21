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

    for (int board = 0; board < NUM_BOARDS; ++board)
    {
        batt_gen_pec((uint8_t*) &(writeData[board]), writeDataSize, data_PEC);
        memcpy(&txBuffer[txBufferIndex], (uint8_t*) &(writeData[board]), writeDataSize);
        txBufferIndex += 6;
        memcpy(&txBuffer[txBufferIndex], data_PEC, 2);
        txBufferIndex += 2;
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
        DEBUG_PRINT("%u != %u. %u != %u\r\n", pec[0],  rxBuffer[pec_index], pec[1], rxBuffer[pec_index + 1]);
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
    return status;
}

HAL_StatusTypeDef batt_spi_tx(uint8_t *txBuffer, size_t len)
{
    HAL_GPIO_WritePin(ISO_SPI_NSS_GPIO_Port, ISO_SPI_NSS_Pin, GPIO_PIN_RESET);
    HAL_StatusTypeDef status = HAL_SPI_Transmit(&ISO_SPI_HANDLE, txBuffer, len, HSPI_TIMEOUT);
    delay_us(2); // t6 = (70ns * numDevices) + 950ns
    HAL_GPIO_WritePin(ISO_SPI_NSS_GPIO_Port, ISO_SPI_NSS_Pin, GPIO_PIN_SET);
    delay_us(2); // t5 = (70ns * numDevices) + 900ns
    return status;
}

// Wake up the spi bus
// @param standby: Set to true if the core state is standby, the device wakes faster in this case
static uint32_t lastWakeup_ticks = 0;
int batt_spi_wakeup(bool sleeping)
{
    if (!sleeping) {
        // The boards have been initialized, so we're in the REFUP state
        if (xTaskGetTickCount() - lastWakeup_ticks <= pdMS_TO_TICKS(US_TO_MS(T_IDLE_US))) {
            // SPI bus already up
            return 0;
        }
    }

    // Send some dummy bytes to wake up SPI/LTC
    uint8_t dummy = 0xFF;

    // Wake up the serial interface on device S1.
    if (sleeping) {
		HAL_GPIO_WritePin(ISO_SPI_NSS_GPIO_Port, ISO_SPI_NSS_Pin, GPIO_PIN_RESET);
		/* TODO: take out of common file */
        long_delay_us(LTC_T_WAKE_MAX_US * NUM_BOARDS); // Have to be careful here, with 14 AMS boards we wait 2.3ms and t_IDLE is 2.4ms
		HAL_GPIO_WritePin(ISO_SPI_NSS_GPIO_Port, ISO_SPI_NSS_Pin, GPIO_PIN_SET);

#if (LTC_T_WAKE_MAX_US * NUM_BOARDS >= T_IDLE_US) // Included as suggested by datasheet
        if (batt_spi_tx(&dummy, JUNK_SIZE))
        {
            ERROR_PRINT("Failed to wake isospi after wake to core\n");
            return 2;
        }
        long_delay_us(NUM_BOARDS * LTC_T_READY_US); // Have to be careful here, with 14 AMS boards we wait 2.3ms and t_IDLE is 2.4ms
#endif
    }
	else{
		if (batt_spi_tx(&dummy, JUNK_SIZE))
		{
			ERROR_PRINT("Failed to wakeup isospi while not asleep\n");
			return 1;
		}
	}

    lastWakeup_ticks = xTaskGetTickCount();

    return 0;
}

// input voltage is in Volts, precision is in increments of 100uV
// output temp is in degrees C
float batt_convert_voltage_to_temp(float voltage) {
    // for NTCLP100. Raw data will be uploaded to OpenProject under firmware.
    const float p1 = 5.1416;
    const float p2 = -47.6355;
    const float p3 = 182.1670;
    const float p4 = -361.8757;
    const float p5 = 389.5266;
    const float p6 = -182.8840;
    const float p7 = 24.5223;

    float x = voltage;

    float output = p1*pow(x,6) + p2*pow(x,5) + p3*pow(x,4) + p4*pow(x,3) + p5*pow(x,2)
        + p6*pow(x,1) + p7;

    return output;
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
