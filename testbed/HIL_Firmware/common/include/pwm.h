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
#define LEDC_MAX_FREQUENCY (40000000) // Maximum frequency in Hz, 40 MHz

typedef enum 
{
    PWM_CHANNEL_INDEX_IMD = 0U,
    CHANNEL_INDEX_NUM // THIS HAS TO BE THE LAST ENTRY
} Channel_Index_E;

// Function prototypes
void pwm_init(void);
void pwm_set_pin(Channel_Index_E channel_index, uint8_t pin_num, uint32_t freq_hz, uint16_t duty_percent); // Corrected prototype
void pwm_simulate_imd_status(uint32_t freq_mhz, uint32_t duty_percent);

#endif // PWM_H
