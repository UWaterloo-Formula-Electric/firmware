#include "state_of_charge_data.h"

//The index is calculated using the number of 0.1V steps above 56V the reading is
const float highVoltageSocLut[HV_SOC_LUT_LEN] = 
{
    0.94200, //56.0 V
    0.95159, //56.1 V
    0.95786, //56.2 V
    0.96432, //56.3 V
    0.96929, //56.4 V
    0.97421, //56.5 V
    0.97783, //56.6 V
    0.98146, //56.7 V
    0.98464, //56.8 V
    0.98679, //56.9 V
    0.98895, //57.0 V
    0.99110, //57.1 V
    0.99325, //57.2 V
    0.99405  //57.3 V
};

const float midVoltageSocLut[MID_SOC_LUT_LEN] = 
{
    0.13283,	// 45V
    0.16790,	// 46V
    0.22169,	// 47V
    0.29539,	// 48V
    0.38091,	// 49V
    0.45930,	// 50V
    0.52878,	// 51V
    0.60245,	// 52V
    0.68764,	// 53V
    0.75858,	// 54V
    0.82780,	// 55V
    0.94200,	// 56V
    0.98895,	// 57V
};

//the index is calculated using the number of volts above 36V the reading is
const float lowVoltageSocLut[LV_SOC_LUT_LEN] = 
{
    0.01114,  //38V
    0.02005,  //39V
    0.03124,  //40V
    0.04488,  //41V 
    0.06144,  //42V 
    0.08186,  //43V 
    0.10583,  //44V 
    0.13283,  //45V 
    0.16790   //46V
};

