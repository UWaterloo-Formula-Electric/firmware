#include "cmsis_os.h"
#include <math.h>
#include <string.h>
#include <stdio.h>
#include "waterflowtempsensor.h"
#include "debug.h"

/**
 * @brief Creates queues for flow rate measurements (uses L/min for units)
 * Queue overwrite, Queue peek, and queue size of 1 is used to ensure only the most recent
 * data is in the queue and read from the queue.
 */
HAL_StatusTypeDef initFlowRateQueue()
{
   flowRateQueue = xQueueCreate(1, sizeof(int));

   if (flowRateQueue == NULL) {
      ERROR_PRINT("Failed to create flow rate queue!\n");
      return HAL_ERROR;
   }

   return HAL_OK;
}

// TODO: add error handling for queue functions
//void updateFlowRate() {
//	int pulseCount; 
//	int newPulseCount = 0;
//	xQueuePeek(flowPulseCountQueue, &pulseCount, 0);
//	xQueueOverwrite(flowPulseCountQueue, &newPulseCount);
//	double flowRate = (1 / pulseCount) / flowRateFactor;
//	xQueueOverwrite(flowRateQueue, &flowRate);
//}

double readFlowRate() {
	double flowRate;
	xQueuePeek(flowRateQueue, &flowRate, 0);
	return flowRate;
}

double readThermistor(){
	  int num = 10;
	  double adcAverage = 0;
	  uint16_t adcSamples[num];

	  for(int i=0;i<num;i++){
		  //start the ADC
		  HAL_ADC_Start(&hadc1);
		  //get value from ADC
		  //HAL_OK is a predefined constant in the HAL library, indicating that the ADC conversion was successful
		  if(HAL_ADC_PollForConversion(&hadc1, HAL_MAX_DELAY) == HAL_OK){
			  adcSamples[i] = HAL_ADC_GetValue(&hadc1);
		  }
		  HAL_ADC_Stop(&hadc1);
		  HAL_Delay(10);
	  }

	  for(int i=0;i<num;i++){
		  adcAverage += adcSamples[i];
	  }
	  adcAverage /= num;
	  return getTemperature(adcAverage);
}


double getTemperature(double adcAverage){
	  double rThermistor;
	  double rResistor = 50.0;//unit:Kohm
	  double vInput = 3.3;
	  double temperature;
	  double tRoom = 298.15;//Kelvin  25Celsius
	  double beta = 3950.0;//(K)
	  double rRoom = 50.0;//Kohm

	  rThermistor = rResistor * ((vInput/adcAverage) + 1);
	  //Voltage division
	  temperature = ((beta * tRoom)/(beta + tRoom * log(rThermistor/rRoom)))-275.15;
	  return temperature;
}

void StartWaterflowTempSensorTask(void const * argument)
{

  /* USER CODE BEGIN 5 */
	double currentTemperature = 0.0;
	char msg[50];
  /* Infinite loop */
  while (1)
  {

	  currentTemperature = readThermistor();

	  // Transmit temperature data over UART
	  sprintf(msg, "Temperature: %f\r\n", currentTemperature);
	  HAL_UART_Transmit(&huart2, (uint8_t *)msg, strlen(msg), HAL_MAX_DELAY);

	  // Transmit flow rate data over UART
	  sprintf(msg, "Flow Rate: %f L/min\r\n", readFlowRate());
	  HAL_UART_Transmit(&huart2, (uint8_t *)msg, strlen(msg), HAL_MAX_DELAY);

	  HAL_Delay(1000);
	/* USER CODE END WHILE */

	/* USER CODE BEGIN 3 */
  }
  /* USER CODE END 5 */
}
