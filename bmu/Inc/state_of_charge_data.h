#ifndef STATE_OF_CHARGE_DATA_H
#define STATE_OF_CHARGE_DATA_H

#define TEMP_LUT_LEN 8U
#define OCV_LUT_SOC_POINTS 21U

extern const float TEMP_LUT[TEMP_LUT_LEN];
extern const float OCV_LUT[OCV_LUT_SOC_POINTS][TEMP_LUT_LEN];
#endif
