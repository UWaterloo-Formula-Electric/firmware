#include "main.h"
#include "cmsis_os.h"
#include <math.h>
#include <string.h>
#include <stdio.h>

ADC_HandleTypeDef hadc1;
UART_HandleTypeDef huart2;

void StartWaterflowTempSensorTask(void const * argument);
double readThermistor(void);
double getTemperature(double adcAverage);

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

	  HAL_Delay(1000);
	/* USER CODE END WHILE */

	/* USER CODE BEGIN 3 */
  }
  /* USER CODE END 5 */
}
