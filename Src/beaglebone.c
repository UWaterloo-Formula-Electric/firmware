#include "beaglebone.h"

HAL_StatusTypeDef beagleboneOff()
{
    PP_BB_DISABLE;
    PP_5V0_DISABLE;

    return HAL_OK;
}

HAL_StatusTypeDef beaglebonePower(bool enable)
{
  if (enable) {
    PP_5V0_ENABLE;
    HAL_Delay(100);
    PP_BB_ENABLE;
  } else {
    //TODO: send shutdown message to BB
    PP_BB_DISABLE;
    HAL_Delay(100);
    PP_5V0_DISABLE;
  }

  return HAL_OK;
}
