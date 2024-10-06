#include "state_of_charge_data.h"

//The index is calculated using the number of 0.1V steps above 56V the reading is
const float highVoltageSocLut[HV_SOC_LUT_LEN] = 
{
    0.94200, // 4V per cell
    0.95159, 
    0.95786, 
    0.96432, 
    0.96929, 
    0.97421, 
    0.97783, 
    0.98146, 
    0.98464, 
    0.98679, 
    0.98895, 
    0.99110, 
    0.99325, 
    0.99405  // 4.1V per cell
};

const float midVoltageSocLut[MID_SOC_LUT_LEN] = 
{
    0.13283, // 3.21V per cell
    0.16790,
    0.22169,
    0.29539,
    0.38091,
    0.45930,
    0.52878,
    0.60245,
    0.68764,
    0.75858,
    0.82780,
    0.94200,
    0.98895, // 4.07V per cell
};

//the index is calculated using the number of volts above 36V the reading is
const float lowVoltageSocLut[LV_SOC_LUT_LEN] = 
{
    0.01114, // 2.71V per cell
    0.02005,
    0.03124,
    0.04488,
    0.06144,
    0.08186,
    0.10583,
    0.13283,
    0.16790  // 3.28V per cell
};

