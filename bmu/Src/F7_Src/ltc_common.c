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
    uint32_t pec_loc = dataSize;
    if (pec[0] == rxBuffer[pec_loc] && pec[1] == rxBuffer[pec_loc + 1])
    {
        return HAL_OK;
    } else {
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
    HAL_GPIO_WritePin(ISO_SPI_NSS_GPIO_Port, ISO_SPI_NSS_Pin, GPIO_PIN_SET);
    return status;
}

HAL_StatusTypeDef batt_spi_tx(uint8_t *txBuffer, size_t len)
{
    HAL_GPIO_WritePin(ISO_SPI_NSS_GPIO_Port, ISO_SPI_NSS_Pin, GPIO_PIN_RESET);
    HAL_StatusTypeDef status = HAL_SPI_Transmit(&ISO_SPI_HANDLE, txBuffer, len, HSPI_TIMEOUT);
    HAL_GPIO_WritePin(ISO_SPI_NSS_GPIO_Port, ISO_SPI_NSS_Pin, GPIO_PIN_SET);
    return status;
}


// Wake up the spi bus
// @param standby: Set to true if the device is in standby already, the device
// wakes faster in this case
static uint32_t lastWakeup_ticks = 0;
int batt_spi_wakeup(bool sleeping)
{
    if (!sleeping) {
        // The boards have been initialized, so we're in the REFUP state
        if (xTaskGetTickCount() - lastWakeup_ticks <= pdMS_TO_TICKS(T_IDLE_MS)) {
            // SPI bus already up
            return 0;
        }
    }

    // Send some dummy bytes to wake up SPI/LTC
    uint8_t dummy = 0xFF;

    // Wake up the serial interface on device S1.
    if (sleeping) {
        for (int board = 0; board < NUM_BOARDS; board++) {
            HAL_GPIO_WritePin(ISO_SPI_NSS_GPIO_Port, ISO_SPI_NSS_Pin, GPIO_PIN_RESET);
            delay_us(300);
            HAL_GPIO_WritePin(ISO_SPI_NSS_GPIO_Port, ISO_SPI_NSS_Pin, GPIO_PIN_SET);
            delay_us(10);
        }
    }  
    for (int board = 0; board < NUM_BOARDS; board++) {
		if (batt_spi_tx(&dummy, JUNK_SIZE))
		{
			ERROR_PRINT("Failed to wakeup batt spi\n");
			return 1;
		}
	}

    lastWakeup_ticks = xTaskGetTickCount();

    return 0;
}

// input voltage is in Volts, precision is in increments of 100uV
// output temp is in degrees C
float batt_convert_voltage_to_temp(float voltage) {
    const float p1 = 3.18239706;
    const float p2 = -41.7652481;
    const float p3 = 235.0295548;
    const float p4 = -740.589814;
    const float p5 = 1434.20493076;
    const float p6 = -1766.88382998;
    const float p7 = 1394.13426838;
    const float p8 = -700.83913797;
    const float p9 = 250.43297764;
    const float p10 = -56.97182216;

    float x = voltage;

    // Calculated from matlab
    float output = p1*pow(x,9) + p2*pow(x,8) + p3*pow(x,7) + p4*pow(x,6) + p5*pow(x,5)
        + p6*pow(x,4) + p7*pow(x,3) + p8*pow(x,2) + p9*x + p10;

    return output;
}

/* delay function for wakeup */
void delay_us(uint32_t time_us)
{
	__HAL_TIM_SetCounter(&DELAY_TIMER,0);
	__HAL_TIM_SetAutoreload(&DELAY_TIMER,0xffff);
	HAL_TIM_Base_Start(&DELAY_TIMER);
	while(DELAY_TIMER_INSTANCE->CNT < time_us);
	HAL_TIM_Base_Stop(&DELAY_TIMER);
}

