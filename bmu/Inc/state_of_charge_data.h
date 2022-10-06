#ifndef STATE_OF_CHARGE_DATA_H

#define STATE_OF_CHARGE_DATA_H
#define HV_SOC_LUT_MIN (56.0f)
#define HV_SOC_LUT_LEN 14U
#define HV_SOC_LUT_STEP 0.1f

#define LV_SOC_LUT_MIN (38.0f)
#define LV_SOC_LUT_LEN 9U
#define LV_SOC_LUT_STEP 1.f

#define MID_SOC_LUT_LEN 13U
#define MID_SOC_LUT_STEP 1.0f
#define MID_SOC_LUT_MIN (45.0f)

extern const float highVoltageSocLut[HV_SOC_LUT_LEN];
extern const float midVoltageSocLut[MID_SOC_LUT_LEN];
extern const float lowVoltageSocLut[LV_SOC_LUT_LEN];
#endif
