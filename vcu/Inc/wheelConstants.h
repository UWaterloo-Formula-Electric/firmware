#ifndef __WHEELCONSTANTS_H__
#define __WHEELCONSTANTS_H__
#include "mathUtils.h"

// Macros for converting RPM to KPH
#define WHEEL_DIAMETER_IN (16.0f) // 16 inch wheel diameter
#define WHEEL_DIAMETER_M (WHEEL_DIAMETER_IN * 2.54 / 100)
#define GEAR_RATIO_MOT_TO_WHEEL 4.f
#define WHEEL_CIRCUMFRENCE (WHEEL_DIAMETER_M * M_PI) // in meters
#define M_PER_KM 1000.0f
#define MIN_PER_HR 60.0f
#define RPM_TO_KPH ((WHEEL_CIRCUMFRENCE / GEAR_RATIO_MOT_TO_WHEEL) / M_PER_KM * MIN_PER_HR)


// Macros for converting RADS to KPH
#define RPM_TO_RAD (2*M_PI/60)
#define RADS_TO_RPM (1/RPM_TO_RAD)
#define GEAR_RATIO (1/GEAR_RATIO_MOT_TO_WHEEL)
#define RADS_TO_KPH(rads) ((rads) * RADS_TO_RPM * RPM_TO_KPH)

#endif // __WHEELCONSTANTS_H__