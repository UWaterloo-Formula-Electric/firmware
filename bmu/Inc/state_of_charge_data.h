#ifndef STATE_OF_CHARGE_DATA_H

#define STATE_OF_CHARGE_DATA_H

#define HV_SOC_LUT_MIN (80.0f)
#define HV_SOC_LUT_LEN 14U
#define HV_SOC_LUT_STEP 0.1f

#define MID_SOC_LUT_LEN 13U
#define MID_SOC_LUT_STEP 1.0f
#define MID_SOC_LUT_MIN (64.2f)

#define LV_SOC_LUT_MIN (54.2f)
#define LV_SOC_LUT_LEN 9U
#define LV_SOC_LUT_STEP 1.f

extern const float highVoltageSocLut[HV_SOC_LUT_LEN];
extern const float midVoltageSocLut[MID_SOC_LUT_LEN];
extern const float lowVoltageSocLut[LV_SOC_LUT_LEN];
#endif
