#include "debug.h"
#include "bsp.h"
#include "stdio.h"
#include "string.h"

#define BUFFER_SIZE (100)

// create a global variable in your program called "PS" to overwrite this,
// DO NOT change this
__weak char PS[] = "PROC";

uint8_t rxBuffer = '\000';
static char rxString[BUFFER_SIZE];
static int rxindex = 0;

static char commandStr[BUFFER_SIZE];
static int commandLen = 0;

void print_now(char str[]);
__weak void executeSerialCommand(char str[]);

int _write(int file, char* data, int len) {
    HAL_UART_Transmit(&DEBUG_UART_HANDLE, (uint8_t*)data, len, UART_PRINT_TIMEOUT);
    return len;
}

HAL_StatusTypeDef uartStartReceiving(UART_HandleTypeDef *huart) {
    __HAL_UART_FLUSH_DRREGISTER(huart); // Clear the buffer to prevent overrun
    return HAL_UART_Receive_DMA(huart, &rxBuffer, 1);
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    __HAL_UART_FLUSH_DRREGISTER(huart); // Clear the buffer to prevent overrun

    int i = 0;

    print_now((char *)&rxBuffer); // Echo the character that caused this callback so the user can see what they are typing

    if (rxBuffer == 8 || rxBuffer == 127) // If Backspace or del
    {
        print_now(" \b"); // "\b space \b" clears the terminal character. Remember we just echoced a \b so don't need another one here, just space and \b
        rxindex--; 
        if (rxindex < 0) rxindex = 0;
    }

    else if (rxBuffer == '\n' || rxBuffer == '\r') // If Enter
    {
        print_now("\n");
        strlcpy(commandStr, rxString, rxindex + 1);
        commandLen = rxindex;
        executeSerialCommand(rxString);
        rxString[rxindex] = 0;
        rxindex = 0;
        for (i = 0; i < BUFFER_SIZE; i++) rxString[i] = 0; // Clear the string buffer
        print_now("\r\n");
        print_now(PS);
        print_now("> ");
    }

    else
    {
        rxString[rxindex] = (char) rxBuffer; // Add that character to the string
        rxindex++;
        if (rxindex > BUFFER_SIZE) // User typing too much, we can't have commands that big
        {
            rxindex = 0;
            for (i = 0; i < BUFFER_SIZE; i++) rxString[i] = 0; // Clear the string buffer
            print_now("\r\n");
            print_now(PS);
            print_now("> ");
        }
    }
}

void print_now(char str[]) {
  HAL_UART_Transmit(&DEBUG_UART_HANDLE, (uint8_t *)str, strlen(str), 5);
}

// implement this function in your code to act on commands received from the
// serial port
// DO NOT change this empty implementation
__weak void executeSerialCommand(char str[]) {
    DEBUG_PRINT("Serial command: %s\n", str);
}
