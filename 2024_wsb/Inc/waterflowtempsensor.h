#include "stm32f4xx_hal.h"
#include "queue.h"

ADC_HandleTypeDef hadc1;
UART_HandleTypeDef huart2;

void StartWaterflowTempSensorTask(void const * argument);
double readThermistor(void);
double getTemperature(double adcAverage);
HAL_StatusTypeDef initFlowRateQueue();
double readFlowRate();

const double flowRateFactor = 11.0; // F = 11.0 * Q, F = frequency (Hz), Q = Flow Rate (L/min)
// Queue holding most recent pulse count and calculated flow rate for flow rate meter
QueueHandle_t flowRateQueue;