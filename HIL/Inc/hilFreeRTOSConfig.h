/**
  *****************************************************************************
  * @file    hilFreeRTOSConfig.h
  * @brief   FreeRTOS settings that CubeMX would otherwise own.
  * @details Cube-F7-Src-respin/Core/Inc/FreeRTOSConfig.h is rewritten on every
  * code generation, so anything put in its managed region is lost. That file
  * includes this one from its USER CODE Defines section, which does survive.
  * Keep FreeRTOS configuration here, not there.
  *****************************************************************************
  */

#ifndef HIL_FREERTOS_CONFIG_H
#define HIL_FREERTOS_CONFIG_H

#include <stdint.h>

// Run-time stats. Implemented in common/Src/debug.c behind
// #ifdef STATS_TIM_HANDLE, which Inc/bsp.h points at TIM7.
#ifndef configGENERATE_RUN_TIME_STATS
#define configGENERATE_RUN_TIME_STATS            1
#endif
#ifndef configUSE_TRACE_FACILITY
#define configUSE_TRACE_FACILITY                 1
#endif
#ifndef configUSE_STATS_FORMATTING_FUNCTIONS
#define configUSE_STATS_FORMATTING_FUNCTIONS     1
#endif

extern void configureTimerForRunTimeStats(void);
extern uint32_t getRunTimeCounterValue(void);

#define portCONFIGURE_TIMER_FOR_RUN_TIME_STATS configureTimerForRunTimeStats
#define portGET_RUN_TIME_COUNTER_VALUE getRunTimeCounterValue

#endif /* HIL_FREERTOS_CONFIG_H */
