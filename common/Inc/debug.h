#ifndef __DEBUG_H
#define __DEBUG_H
#include "bsp.h"
#include "usart.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "stdio.h"
#include "generalErrorHandler.h"

#ifdef DEBUG_ON
#define DEBUG_PRINT(...) _DEBUG_PRINT(__VA_ARGS__)
#define DEBUG_PRINT_ISR(...) _DEBUG_PRINT_ISR(__VA_ARGS__)
#else
#define DEBUG_PRINT(...)
#define DEBUG_PRINT_ISR(...)
#endif

#ifdef ERROR_PRINT_ON
#define ERROR_PRINT(...) _DEBUG_PRINT(__VA_ARGS__)
#define ERROR_PRINT_ISR(...) _DEBUG_PRINT_ISR(__VA_ARGS__)
#else
#define ERROR_PRINT(...)
#endif

#ifdef CONSOLE_PRINT_ON
#define CONSOLE_PRINT(...) _DEBUG_PRINT(__VA_ARGS__)
#else
#define CONSOLE_PRINT(...)
#endif

/*
 * Send Function and Defines
 */
#define UART_PRINT_TIMEOUT 100

#if IS_BOARD_F7_FAMILY
// We have more space on F7, so make this longer
#define PRINT_QUEUE_LENGTH 15
#elif IS_BOARD_F0_FAMILY
#define PRINT_QUEUE_LENGTH 5
#endif

#define PRINT_QUEUE_STRING_SIZE 100
#define PRINT_QUEUE_SEND_TIMEOUT_TICKS  10

/**
 * @brief Send a debug string to the uart
 *
 * @param ... the format string and any arguments
 *
 * This creates a string from the format and arguments, then copies it to a
 * queue for future printing over the uart by the debug task
 * This will trim any resultant string to PRINT_QUEUE_STRING_SIZE characters
 * It will silently fail if the queue is full after waiting for
 * QUEUE_SEND_TIMEOUT_TICKS
 *
 * @return Nothing
 */
#define _DEBUG_PRINT(...) \
    do { \
        if (!printQueue) { \
            handleError(); \
        } \
        char buf[PRINT_QUEUE_STRING_SIZE] = {0}; \
        snprintf(buf, PRINT_QUEUE_STRING_SIZE, __VA_ARGS__); \
        xQueueSend(printQueue, buf, PRINT_QUEUE_SEND_TIMEOUT_TICKS); \
    } while(0)

/**
 * @brief Send a debug string to the uart
 *
 * @param ... the format string and any arguments
 *
 * This creates a string from the format and arguments, then copies it to a
 * queue for future printing over the uart by the debug task
 * This will trim any resultant string to PRINT_QUEUE_STRING_SIZE characters
 * It will silently fail if the queue is full after waiting for
 * QUEUE_SEND_TIMEOUT_TICKS
 *
 * @return Nothing
 */
#define _DEBUG_PRINT_ISR(...) \
    do { \
        if (!printQueue) { \
            handleError(); \
        } \
        char buf[PRINT_QUEUE_STRING_SIZE] = {0}; \
        snprintf(buf, PRINT_QUEUE_STRING_SIZE, __VA_ARGS__); \
        xQueueSendFromISR(printQueue, buf, NULL); \
    } while(0)

extern QueueHandle_t printQueue;
extern QueueHandle_t uartRxQueue;

extern uint8_t isUartOverCanEnabled;
/* 
 * Receive and CLI functions and defines
 *
 */
// This is the size of the buffer used by FreeRTOS+CLI to write command output
// to
// this **NEEDS* to be the same as the print queue string size, as the console
// send function relies on this. This optimizes sending
#define configCOMMAND_INT_MAX_OUTPUT_SIZE PRINT_QUEUE_STRING_SIZE

#define UART_RX_QUEUE_LENGTH 100

#define STR_EQ(a, b, len) (strncmp(a, b, len) == 0)

// Output to the command output buffer, can only be called once per command
// function
#define COMMAND_OUTPUT(...)  \
    do { \
        snprintf(writeBuffer, writeBufferLength, __VA_ARGS__); \
    } while(0)

HAL_StatusTypeDef debugInit();
HAL_StatusTypeDef uartStartReceiving(UART_HandleTypeDef *huart);

#endif /* define(__DEBUG_H) */
