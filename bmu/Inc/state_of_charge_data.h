#include "F7_Inc/ltc_chip.h"

#ifndef STATE_OF_CHARGE_DATA_H

#define STATE_OF_CHARGE_DATA_H

#define HV_SOC_LUT_MIN (4.0f * CELLS_PER_BOARD * NUM_BOARDS_PER_SEGMENT)
#define HV_SOC_LUT_LEN 14U
#define HV_SOC_LUT_STEP 0.1f

#define MID_SOC_LUT_MIN (3.21f * CELLS_PER_BOARD * NUM_BOARDS_PER_SEGMENT)
#define MID_SOC_LUT_LEN 13U
#define MID_SOC_LUT_STEP 1.0f

#define LV_SOC_LUT_MIN (2.71f * CELLS_PER_BOARD * NUM_BOARDS_PER_SEGMENT)
#define LV_SOC_LUT_LEN 9U
#define LV_SOC_LUT_STEP 1.0f

#define TEMP_LUT_LEN 8U
#define OCV_LUT_SOC_POINTS 21U

extern const float highVoltageSocLut[HV_SOC_LUT_LEN];
extern const float midVoltageSocLut[MID_SOC_LUT_LEN];
extern const float lowVoltageSocLut[LV_SOC_LUT_LEN];
extern const float TEMP_LUT[TEMP_LUT_LEN];
extern const float OCV_LUT[OCV_LUT_SOC_POINTS][TEMP_LUT_LEN];
#endif
