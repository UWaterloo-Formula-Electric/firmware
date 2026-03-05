#ifndef STATE_OF_HEALTH_H
#define STATE_OF_HEALTH_H

// This structure tracks the estimated battery capacity and the filter's uncertainty
typedef struct {
    float capacity_ah;          // Current estimated capacity 
    float nominal_capacity_ah;  // Factory-rated capacity 
    float P;                    // Error covariance (uncertainty) of the estimate
} state_of_health_t;

// Initialize SOH EKF
// Sets the initial capacity to nominal and initializes the uncertainty (P)
void soh_init(state_of_health_t *soh, float nominal_capacity_ah);

// EKF update
// called periodically. Note that voltage_v must be normalized to the cell level (approx. 2.7V - 4.2V)
void soh_update_ekf(state_of_health_t *soh, float soc, float current_a, float voltage_v, float dt_s);

// Get SOH
float soh_get(const state_of_health_t *soh);

#endif