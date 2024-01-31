#ifndef PWM_H
#define PWM_H

#include "driver/ledc.h"
#include "esp_err.h"

// Constants for LEDC configuration
#define LEDC_CHANNEL    LEDC_CHANNEL_0
#define LEDC_TIMER      LEDC_TIMER_0
#define LEDC_MODE       LEDC_LOW_SPEED_MODE
#define LEDC_GPIO       (5)  // GPIO number
#define LEDC_FREQUENCY  (10000)  // Default frequency in Hz

// Function prototypes
void pwm_init(void);
esp_err_t pwm_set_pin(uint8_t pin_num, uint32_t freq_hz, uint16_t duty_percent);
void pwm_simulate_imd_status(uint32_t freq_mhz, uint32_t duty_percent);

#endif // PWM_H
