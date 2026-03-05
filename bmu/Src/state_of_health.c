#include "state_of_health.h"
#include "state_of_charge_data.h"
#include <math.h>

// Tuning params
#define CAPACITY_PROCESS_NOISE   1e-9f
#define VOLTAGE_MEAS_NOISE       5e-2f   // V^2

// ECM placeholders 
#define R0 0.0f   // Ohmic resistance (fill in later)

// Helper function: linear interpolation for OCV and slope
// Given SOC, find OCV and dOCV/dSOC using soc data LUTs
static void get_ocv_params(float soc, float *ocv, float *slope) {
    // Treat the LUTs as a combined dataset
    // Assuming these are "Per Cell" voltages for your algorithm:

    // Low Range: 2.71V to 3.28V (9 points)
    float low_volts[LV_SOC_LUT_LEN] = {2.71, 2.78, 2.85, 2.92, 2.99, 3.06, 3.13, 3.21, 3.28};
    
    // Mid Range: 3.21V to 4.07V (13 points)
    // We'll distribute these linearly between the start and end points provided
    float mid_volts[MID_SOC_LUT_LEN];
    for(int i = 0; i < MID_SOC_LUT_LEN; i++) {
        mid_volts[i] = 3.21f + i * (4.07f - 3.21f) / (MID_SOC_LUT_LEN - 1);
    }

    // High Range: 4.0V to 4.1V (14 points)
    float high_volts[HV_SOC_LUT_LEN];
    for(int i = 0; i < HV_SOC_LUT_LEN; i++) {
        high_volts[i] = 4.00f + (i * 0.01f); 
    }


    // --- SEARCH LOGIC ---
    
    // 1. Check High Range First (highest precision)
    if (soc >= highVoltageSocLut[0]) {
        for (int i = 0; i < HV_SOC_LUT_LEN - 1; i++) {
            if (soc <= highVoltageSocLut[i+1]) {
                float dSOC = highVoltageSocLut[i+1] - highVoltageSocLut[i];
                *slope = 0.01f / (dSOC > 1e-6f ? dSOC : 1e-6f); 
                *ocv = high_volts[i] + (*slope * (soc - highVoltageSocLut[i]));
                return;
            }
        }
    }
    
    // 2. Check Mid Range
    if (soc >= midVoltageSocLut[0]) {
        for (int i = 0; i < MID_SOC_LUT_LEN - 1; i++) {
            if (soc <= midVoltageSocLut[i+1]) {
                float dV = mid_volts[i+1] - mid_volts[i];
                float dSOC = midVoltageSocLut[i+1] - midVoltageSocLut[i];
                *slope = dV / (dSOC > 1e-6f ? dSOC : 1e-6f);
                *ocv = mid_volts[i] + (*slope * (soc - midVoltageSocLut[i]));
                return;
            }
        }
    }

    // 3. Check Low Range
    if (soc <= lowVoltageSocLut[LV_SOC_LUT_LEN-1]) {
        for (int i = 0; i < LV_SOC_LUT_LEN - 1; i++) {
            if (soc >= lowVoltageSocLut[i] && soc <= lowVoltageSocLut[i+1]) {
                float dV = low_volts[i+1] - low_volts[i];
                float dSOC = lowVoltageSocLut[i+1] - lowVoltageSocLut[i];
                *slope = dV / (dSOC > 1e-6f ? dSOC : 1e-6f);
                *ocv = low_volts[i] + (*slope * (soc - lowVoltageSocLut[i]));
                return;
            }
        }
    }

    // Fallback
    *ocv = 3.7f; 
    *slope = 0.5f;
}


// Init
void soh_init(state_of_health_t *soh, float nominal_capacity_ah) {
    soh->nominal_capacity_ah = nominal_capacity_ah;
    soh->capacity_ah = nominal_capacity_ah; // Start at 100% SOH
    soh->P = 1.0f; // Start with higher uncertainty
}


// EKF update
void soh_update_ekf(state_of_health_t *soh, float soc, float current_a, float voltage_v, float dt_s) {
    // 1. Prediction
    float Q_pred = soh->capacity_ah;
    float P_pred = soh->P + CAPACITY_PROCESS_NOISE;

    // 2. Get OCV and Slope from SOC
    float ocv, dOCV_dSOC;
    get_ocv_params(soc, &ocv, &dOCV_dSOC);

    // 3. Measurement Model (V = OCV - I*R)
    float v_pred = ocv - (current_a * R0);
    float residual = voltage_v - v_pred;

    // 4. Jacobian Calculation 
    // Relate Terminal Voltage to Capacity
    // Since SOC = Q_remaining / Q_total, then dSOC/dQ = -SOC / Q
    float dSOC_dQ = -soc / Q_pred;
    float H = dOCV_dSOC * dSOC_dQ;

    // 5. Kalman gain
    float S = H * P_pred * H + VOLTAGE_MEAS_NOISE;
    float K = (fabsf(S) > 1e-6f) ? (P_pred * H / S) : 0.0f;

    // 6. Update
    soh->capacity_ah = Q_pred + K * residual;
    soh->P = (1.0f - K * H) * P_pred;

    // 7. Safety clamp
    if (soh->capacity_ah > soh->nominal_capacity_ah * 1.1f) {
        soh->capacity_ah = soh->nominal_capacity_ah * 1.1f;
    }
    if (soh->capacity_ah < soh->nominal_capacity_ah * 0.5f) {
        soh->capacity_ah = soh->nominal_capacity_ah * 0.5f;
    }
}


// SOH output
float soh_get(const state_of_health_t *soh) {
    if (soh->nominal_capacity_ah <= 0.0f) return 0.0f;
    return (soh->capacity_ah / soh->nominal_capacity_ah) * 100.0f;
}