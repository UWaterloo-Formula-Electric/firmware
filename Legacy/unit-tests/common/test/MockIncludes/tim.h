#ifndef FAKE_TIMER_H
#define FAKE_TIMER_H
#include <stdint.h>
#include "fake_hal_defs.h"

typedef void* TimerHandle_t;
typedef void (*TimerCallbackFunction_t)( TimerHandle_t xTimer );

#if( configUSE_16_BIT_TICKS == 1 )
	typedef uint16_t TickType_t;
	#define portMAX_DELAY ( TickType_t ) 0xffff
#else
	typedef uint32_t TickType_t;
	#define portMAX_DELAY ( TickType_t ) 0xffffffffUL

	/* 32-bit tick type on a 32-bit architecture, so reads of the tick count do
	not need to be guarded with a critical section. */
	#define portTICK_TYPE_IS_ATOMIC 1
#endif


TickType_t xTaskGetTickCount( void );

void fake_mock_init_timers(void);

TimerHandle_t xTimerCreate(	const char * const pcTimerName,
								const TickType_t xTimerPeriodInTicks,
								const UBaseType_t uxAutoReload,
								void * const pvTimerID,
								TimerCallbackFunction_t pxCallbackFunction );

BaseType_t xTimerStart(TimerHandle_t timer, const TickType_t xOptionalValue);
BaseType_t xTimerStop(TimerHandle_t timer, const TickType_t xOptionalValue);

#define xTimerStartFromISR(tim, xopt) (xTimerStart(tim, 0))

#endif
