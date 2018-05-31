#include "stm32f7xx_hal.h"
#include "debug.h"
#include "bsp.h"

int _write(int file, char* data, int len) {
    HAL_UART_Transmit(&DEBUG_UART_HANDLE, (uint8_t*)data, len, UART_PRINT_TIMEOUT);
    return len;
}
