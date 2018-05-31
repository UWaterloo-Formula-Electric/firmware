#ifndef __DEBUG_H
#define __DEBUG_H
#include "bsp.h"
#include "usart.h"

#ifdef DEBUG_ON
#define DEBUG_PRINT(...) printf(__VA_ARGS__)
#else
#define DEBUG_PRINT(...)
#endif

#ifdef ERROR_PRINT_ON
#define ERROR_PRINT(...) printf(__VA_ARGS__)
#else
#define ERROR_PRINT(...)
#endif

#define UART_PRINT_TIMEOUT 1000

#endif /* define(__DEBUG_H)
