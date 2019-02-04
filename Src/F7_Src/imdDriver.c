#include "imdDriver.h"
#include "bsp.h"
#include "debug.h"
#include "BMU_dtc.h"
#include "state_machine.h"
#include "controlStateMachine.h"

/* Captured Value */
__IO uint32_t            meas_IC2_val = 0;
/* Duty Cycle Value */
__IO uint32_t            meas_duty_cycle = 0;
/* Frequency Value */
__IO uint32_t            meas_freq = 0;

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

  return HAL_OK;
}

HAL_StatusTypeDef begin_imd_measurement(void) {
  meas_freq = 0;
  meas_duty_cycle = 0;
  meas_IC2_val = 0;

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
    .meas_freq = meas_freq,
    .status = HAL_GPIO_ReadPin(IMD_SENSE_GPIO_Port, IMD_SENSE_Pin),
  };
}

IMDStatus _imd_status(uint32_t freq, uint32_t duty){
  IMDStatus status = IMDSTATUS_Invalid;

  if ((freq > 9500) && (freq < 10500)){
    if ((duty > 5) && (duty < 95)){
      status = IMDSTATUS_Normal;
    }
  } else if ((freq > 19000) && (freq < 21000)){
    if ((duty > 5) && (duty < 95)){
      status = IMDSTATUS_Undervoltage;
    }
  } else if ((freq > 28500) && (freq < 31500)){
    if ((duty > 5) && (duty < 10)){
      status = IMDSTATUS_SST_Good;
    } else if ((duty > 90) && (duty < 95)){
      status = IMDSTATUS_SST_Bad;
    }
  }else if ((freq > 38000) && (freq < 42000)){
    if ((duty > 47) && (duty < 53)){
      status = IMDSTATUS_Device_Error;
    }
  }else if ((freq > 47500) && (freq < 52500)){
    if ((duty > 47) && (duty < 53)){
      status = IMDSTATUS_Fault_Earth;
    }
  }

  return status;
}

IMDStatus return_imd_status(IMDMeasurements m) {
  return _imd_status(m.meas_freq, m.meas_duty);
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
        meas_duty_cycle = ((HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_1)) * 100) / meas_IC2_val;

        /* meas_freq computation */
        meas_freq = (IMD_TIMER_TICK_FREQUENCY_HZ)  / meas_IC2_val;
      }
      else
      {
        meas_duty_cycle = 0;
        meas_freq = 0;
      }

      IMDStatus status = _imd_status(meas_freq, meas_duty_cycle);

      switch (status) {
        case IMDSTATUS_Normal:
        case IMDSTATUS_SST_Good:
          break;
        case IMDSTATUS_Invalid:
          ERROR_PRINT_ISR("Invalid IMD measurement\n");
          break;
        case IMDSTATUS_Undervoltage:
          ERROR_PRINT_ISR("IMD Status: Undervoltage\n");
          sendDTC_FATAL_IMD_Failure(status);
          break;
        case IMDSTATUS_SST_Bad:
          ERROR_PRINT_ISR("IMD Status: SST_Bad\n");
          sendDTC_FATAL_IMD_Failure(status);
          break;
        case IMDSTATUS_Device_Error:
          ERROR_PRINT_ISR("IMD Status: Device Error\n");
          sendDTC_FATAL_IMD_Failure(status);
          break;
        case IMDSTATUS_Fault_Earth:
          ERROR_PRINT_ISR("IMD Status: Fault Earth\n");
          sendDTC_FATAL_IMD_Failure(status);
          break;
      }

      if (HAL_GPIO_ReadPin(IMD_SENSE_GPIO_Port, IMD_SENSE_Pin) != GPIO_PIN_SET) {
        fsmSendEventUrgentISR(&fsmHandle, EV_HV_Fault);
      }

      if (begin_imd_measurement() != HAL_OK) {
        ERROR_PRINT_ISR("Failed to start next IMD measurement\n");
        fsmSendEventUrgentISR(&fsmHandle, EV_HV_Fault);
      }
    }
  }
}
