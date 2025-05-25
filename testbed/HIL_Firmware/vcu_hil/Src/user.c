#include "debug.h"
#include "userCan.h"
#include "Dac_Driver.h"
#include "canRecieve.h"

void userInit()
{
    if(DAC_Init(&hi2c1, dac1, 1)!= HAL_OK)
    {
        // print("error initialzing DAC")
    }

}