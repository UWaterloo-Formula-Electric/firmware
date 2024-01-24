#include "pwm.h"
#include <stdio.h>

// Initialize LEDC for PWM
void pwm_init() {
    //  existing ledc_init code
    ledc_channel_config_t ledc_channel = {
        .channel    = LEDC_CHANNEL,
        .duty       = 0, // Start with 0% duty cycle
        .gpio_num   = LEDC_GPIO,
        .speed_mode = LEDC_MODE,
        .hpoint     = 0,
        .timer_sel  = LEDC_TIMER
    };
    ledc_timer_config_t ledc_timer = {
        .duty_resolution = LEDC_TIMER_10_BIT,
        .freq_hz         = LEDC_FREQUENCY,
        .speed_mode      = LEDC_MODE,
        .timer_num       = LEDC_TIMER,
        .clk_cfg         = LEDC_AUTO_CLK
    };
}

// Set the PWM duty cycle
esp_err_t pwm_set_duty(uint32_t duty_percent) {
    esp_err_t ret;
    uint32_t duty = 8191 * (duty_percent / 100); // 8191 for 10-bit resolution
    
    // Set the duty cycle 
    ret = ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, duty);
    if (ret != ESP_OK){
        printf("Error setting duty cycle: %s\n", esp_err_to_name(ret));
        return ret;
    }

    // update duty cycle to take effect 
    ret = ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);
    if (ret != ESP_OK) { 
        // Handle error 
        printf("Error updating duty cycle: %s\n", esp_err_to_name(ret));
        return ret;
    } 
    return ESP_OK;
}

// Set the PWM frequency
esp_err_t pwm_set_freq(uint32_t freq_mhz) {
    uint32_t current_freq = ledc_get_freq(LEDC_MODE, LEDC_TIMER);

    // Check if the new frequency is different from the current one
    if (current_freq != freq_mhz) {
        // Attempt to set the new frequency
        esp_err_t ret = ledc_set_freq(LEDC_MODE, LEDC_TIMER, freq_mhz);
        if (ret != ESP_OK) {
            printf("Error setting frequency: %s\n", esp_err_to_name(ret));
            return ret;
        }
    }
    return ESP_OK;
}

// Simulate different IMD statuses
void pwm_simulate_imd_status(uint32_t freq_mhz, uint32_t duty_percent) {
    // Adjust LEDC frequency
    esp_err_t ret = set_pwm_freq(freq_mhz);
    if (ret != ESP_OK) {
        // Handle error
        printf("Error in simulate_imd_status (frequency): %s\n", esp_err_to_name(ret));
        return;
    }

    // Adjust duty cycle
    ret = set_pwm_duty(duty_percent);
    if (ret != ESP_OK) {
        // Handle error
        printf("Error in simulate_imd_status (duty cycle): %s\n", esp_err_to_name(ret));
    }
}
