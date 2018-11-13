#ifndef __DEBUG_H
#define __DEBUG_H
#include "bsp.h"
#include "usart.h"
#include "freertos.h"
#include "queue.h"

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

#define UART_PRINT_TIMEOUT 100

#define PRINT_QUEUE_LENGTH 15
#define PRINT_QUEUE_STRING_SIZE 80
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
            Error_Handler(); \
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
            Error_Handler(); \
        } \
        char buf[PRINT_QUEUE_STRING_SIZE] = {0}; \
        snprintf(buf, PRINT_QUEUE_STRING_SIZE, __VA_ARGS__); \
        xQueueSendFromISR(printQueue, buf, NULL); \
    } while(0)

extern QueueHandle_t printQueue;

HAL_StatusTypeDef debugInit();
HAL_StatusTypeDef uartStartReceiving(UART_HandleTypeDef *huart);

#endif /* define(__DEBUG_H) */
