/**
  *****************************************************************************
  * @file    imdDriver.c
  * @author  Richard Matthews
  * @brief   Module containing functions to read IMD status
  * @details The IMD (Bender IR155-3203/3204) is an insulation monitoring
  * device which monitors the resistance between the HV Bus and the chassis
  * ground. It reports a fault if the insulation drop below a certain value.
  * The IMD communicates with the BMU through a gpio pin indicating a boolean
  * fault status (OK or FAIL) and a PWM signal indicating the fault type.
  *
  ******************************************************************************
  */

#include "imdDriver.h"
#include "bsp.h"
#include "uwfe_debug.h"
#include "bmu_dtc.h"
#include "state_machine.h"
#include "controlStateMachine.h"

#define IMD_SENSE_PIN_FAULT    GPIO_PIN_RESET
#define IMD_SENSE_PIN_NO_FAULT GPIO_PIN_SET

// Lowest freqency expected is 10 Hz, so set timeout to twice that period
#define IMD_FREQ_MEAS_TIMEOUT_MS 200

/* Captured Value */
volatile uint32_t            meas_IC2_val = 0;
volatile uint32_t            meas_IC1_val = 0;

/* Duty Cycle Value */
volatile uint32_t            meas_duty_cycle = 0;
/* Frequency Value */
volatile uint32_t            meas_freq_mHz = 0;

/* Last time we captured a pwm pulse */
volatile uint32_t lastCaptureTimeTicks = 0;

static void print_error(char err[]) {
  ERROR_PRINT("imd error: %s\n", err);
}

#define IMD_TIMER_TICK_FREQUENCY_HZ 50000
HAL_StatusTypeDef init_imd_measurement(void) {
  // Compute the right prescaler to set timer frequency
  RCC_ClkInitTypeDef      clkconfig;
  uint32_t                uwTimclock, uwAPB1Prescaler = 0U;
  uint32_t                uwPrescalerValue = 0U;
  uint32_t                timerFrequency;
  uint32_t                pFLatency;

  /* Get clock configuration */
  HAL_RCC_GetClockConfig(&clkconfig, &pFLatency);

  /* Get APB1 prescaler */
  uwAPB1Prescaler = clkconfig.APB1CLKDivider;

  /* Compute timer clock */
  if (uwAPB1Prescaler == RCC_HCLK_DIV1) 
  {
    uwTimclock = HAL_RCC_GetPCLK1Freq();
  }
  else
  {
    uwTimclock = 2*HAL_RCC_GetPCLK1Freq();
  }

  timerFrequency = IMD_TIMER_TICK_FREQUENCY_HZ;

  /* Compute the prescaler value to have TIM5 counter clock equal to desired
   * freqeuncy*/
  uwPrescalerValue = (uint32_t) ((uwTimclock / timerFrequency) - 1U);

  __HAL_TIM_SET_PRESCALER(&IMD_TIM_HANDLE, uwPrescalerValue);

  /* Zero out values */
  meas_IC2_val = 0;
  meas_IC1_val = 0;

  meas_duty_cycle = 0;
  meas_freq_mHz = 0;

  lastCaptureTimeTicks = 0;

  return HAL_OK;
}

HAL_StatusTypeDef begin_imd_measurement(void) {
  /*##-4- Start the Input Capture in interrupt mode ##########################*/
  if (HAL_TIM_IC_Start_IT(&IMD_TIM_HANDLE, TIM_CHANNEL_2) != HAL_OK)
  {
    /* Starting Error */
    print_error("error starting status timer");
    return HAL_ERROR;
  }

  /*##-5- Start the Input Capture in interrupt mode ##########################*/
  if (HAL_TIM_IC_Start_IT(&IMD_TIM_HANDLE, TIM_CHANNEL_1) != HAL_OK)
  {
    /* Starting Error */
    print_error("error starting status timer");
    return HAL_ERROR;
  }

  return HAL_OK;
}

HAL_StatusTypeDef stop_imd_measurement(void) {
  if (HAL_TIM_IC_Stop_IT(&IMD_TIM_HANDLE, TIM_CHANNEL_2) != HAL_OK) {
    print_error("Failed to stop timer\n");
    return HAL_ERROR;
  }
  if (HAL_TIM_IC_Stop_IT(&IMD_TIM_HANDLE, TIM_CHANNEL_1) != HAL_OK) {
    print_error("Failed to stop timer\n");
    return HAL_ERROR;
  }

  return HAL_OK;
}

IMDMeasurements get_imd_measurements(void) {
  return (IMDMeasurements) {
    .meas_duty = meas_duty_cycle,
    .meas_freq_mHz = meas_freq_mHz,
    .status = HAL_GPIO_ReadPin(IMD_SENSE_GPIO_Port, IMD_SENSE_Pin),
  };
}

IMDStatus _imd_status(uint32_t freq_mHz, uint32_t duty){
  IMDStatus status = IMDSTATUS_Invalid;
  if ((freq_mHz > 9500) && (freq_mHz < 10500)){
    if ((duty > 5) && (duty < 84)){
      status = IMDSTATUS_Normal;
    }
    else if(duty >= 84)
    {
      status = IMDSTATUS_Fault_Earth;
    }
  } else if ((freq_mHz > 19000) && (freq_mHz < 21000)){
    if ((duty > 5) && (duty < 95)){
      status = IMDSTATUS_Undervoltage;
    }
  } else if ((freq_mHz > 28500) && (freq_mHz < 31500)){
    if ((duty > 5) && (duty < 10)){
      status = IMDSTATUS_SST_Good;
    } else if ((duty > 90) && (duty < 95)){
      status = IMDSTATUS_SST_Bad;
    }
  } else if ((freq_mHz > 38000) && (freq_mHz < 42000)){
    if ((duty > 47) && (duty < 53)){
      status = IMDSTATUS_Device_Error;
    }
  } else if ((freq_mHz > 47500) && (freq_mHz < 52500)){
    if ((duty > 47) && (duty < 53)){
      status = IMDSTATUS_Fault_Earth;
    }
  } else{
	  DEBUG_PRINT("--- Unhandled IMD Freq ---\n");
	  DEBUG_PRINT("freq_mHz %lu, duty %lu\n", freq_mHz, duty);
  }

  return status;
}


IMDStatus get_imd_status() {
  IMDMeasurements meas = get_imd_measurements();
  IMDStatus status;

  if (xTaskGetTickCount() - lastCaptureTimeTicks >= pdMS_TO_TICKS(IMD_FREQ_MEAS_TIMEOUT_MS)) {
    // We're not getting pwm pulses anymore, so the frequency is 0 Hz
    // This means a short has occured
    status = IMDSTATUS_HV_Short;

    return status;
  }

  if (meas.status == IMD_SENSE_PIN_FAULT) {
    status = _imd_status(meas.meas_freq_mHz, meas.meas_duty);
  } else {
    status = IMDSTATUS_Normal;
  }

  return status;
}

void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
  if (htim->Instance == IMD_TIM_INSTANCE) { // measurement signal from IMD (meas)
    if (htim->Channel == HAL_TIM_ACTIVE_CHANNEL_2)
    {
      /* Get the Input Capture value */
      meas_IC2_val = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_2);

      if (meas_IC2_val != 0)
      {
        /* Duty cycle computation */
        meas_IC1_val = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_1);

        meas_duty_cycle = (meas_IC1_val * 100) / meas_IC2_val;

        /* meas_freq_mHz computation */
        meas_freq_mHz = (IMD_TIMER_TICK_FREQUENCY_HZ*1000)  / meas_IC2_val;

        lastCaptureTimeTicks = xTaskGetTickCountFromISR();
      }
      else
      {
        meas_duty_cycle = 0;
        meas_freq_mHz = 0;
      }
    }
  }
}
