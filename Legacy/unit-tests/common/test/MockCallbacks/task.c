#include "task.h"
#include <pthread.h>

pthread_t start_task(void *(*task_function)(void *), void* args) {
	pthread_t thread_id;
	pthread_create(&thread_id, NULL, task_function, args); 
	return thread_id;
}

void end_task(pthread_t thread_id) {
	pthread_cancel(thread_id);
}

BaseType_t xTaskGenericNotify( TaskHandle_t xTaskToNotify, uint32_t ulValue, eNotifyAction eAction, uint32_t *pulPreviousNotificationValue ) {
	return 0;
}

BaseType_t xTaskGenericNotifyFromISR( TaskHandle_t xTaskToNotify, uint32_t ulValue, eNotifyAction eAction, uint32_t *pulPreviousNotificationValue, BaseType_t *pxHigherPriorityTaskWoken ) {
	return 0;
}

uint32_t ulTaskNotifyTake( BaseType_t xClearCountOnExit, TickType_t xTicksToWait ) {
	return 0;
}

void vTaskDelay( const TickType_t xTicksToDelay ) {
	// I don't think we need to do anything here
	// If we actually put this here, it will cause tests to take along time
	// It will also not fully emulate STM chip
}

void vTaskDelayUntil( TickType_t * const pxPreviousWakeTime, const TickType_t xTimeIncrement ) {
	// Do nothing
}

TickType_t xTaskGetTickCountFromISR(void) {
	return 0;
}
// Real tick implementations should be created
TickType_t xTaskGetTickCount( void ) {
	return 0;
}

BaseType_t xTaskGetSchedulerState( void ) {
	return taskSCHEDULER_RUNNING;
}
