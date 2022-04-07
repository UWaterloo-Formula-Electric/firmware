#include "state_of_charge_data.h"

//The index is calculated using the number of 0.1V steps above 56V the reading is
const float highVoltageSocLut[HV_SOC_LUT_LEN] = 
{
    0.97106, //56.0 V
    0.97569, //56.1 V
    0.97939, //56.2 V
    0.98266, //56.3 V
    0.98591, //56.4 V
    0.98824, //56.5 V
    0.99058, //56.6 V
    0.99246, //56.7 V
    0.99432, //56.8 V
    0.99573, //56.9 V
    0.99666, //57.0 V
    0.99762, //57.1 V
    0.99855, //57.2 V
    0.99951  //57.3 V
};

const float midVoltageSocLut[MID_SOC_LUT_LEN] = 
{
    0.99666,	// 57V
    0.9508,		// 56V
    0.8424,		// 55V
    0.7637,		// 54V
    0.6949,		// 53V
    0.6100,		// 52V
    0.5300,		// 51V
    0.4620,		// 50V
    0.3860,		// 49V
    0.3030,		// 48V
    0.2300,		// 47V
    0.1760,		// 46V
    0.1380,		// 45V
};

//the index is calculated using the number of volts above 36V the reading is
const float lowVoltageSocLut[LV_SOC_LUT_LEN] = 
{
    0.01158,  //38V
    0.01922,  //39V
    0.02900,  //40V
    0.04138,  //41V 
    0.05681,  //42V 
    0.07612,  //43V 
    0.09806,  //44V 
    0.12527,  //45V 
    0.16476   //46V
};

