#include <stdio.h>
#include "esp_log.h"
#include "driver/ledc.h"
#include "pwm.h"

// Initialize LEDC for PWM
static ledc_channel_config_t ledc_channel_configs[] = {
    // TODO: Update name: CHANNEL_INDEX_TEST
    [PWM_CHANNEL_INDEX_IMD] = 
    {
        .channel    = PWM_CHANNEL_INDEX_IMD,
        .duty       = 0, // Start with 0% duty cycle
        .gpio_num   = 9,
        .speed_mode = LEDC_MODE,
        .hpoint     = 0,
        .timer_sel  = LEDC_TIMER
    },
};

void pwm_init(void) {
    //  existing ledc_init code
    ledc_timer_config_t ledc_timer = {
        .duty_resolution = LEDC_TIMER_10_BIT,
        .freq_hz         = LEDC_FREQUENCY,
        .speed_mode      = LEDC_MODE,
        .timer_num       = LEDC_TIMER,
        .clk_cfg         = LEDC_AUTO_CLK
    };
    ledc_timer_config(&ledc_timer);
}

// Set the PWM duty cycle
void pwm_set_pin(Channel_Index_E channel_index, uint8_t pin_num, uint32_t freq_hz, uint16_t duty_percent) {
    printf("setpwm\r\n");

    // check if the requested frequency exceeds the maximum allowed frequency 
    if (freq_hz > LEDC_MAX_FREQUENCY) {
        printf("Error: Requested frequency %lu Hz exceeds maximum allowed %d Hz\n", freq_hz, LEDC_MAX_FREQUENCY);
        return; // Exit the function early as the requested frequency is not supported
    }

    if (channel_index >= CHANNEL_INDEX_NUM) {
        printf("Error: Invalid channel index\n");
        return;
    }

    // Select the channel configuration based on channel_index
    ledc_channel_config_t* channel_config = &ledc_channel_configs[channel_index];

    // Update the gpio_num based on pin_num if necessary
    channel_config->gpio_num = pin_num;

    // Retrieve current frequency of the timer used by this channel
    uint32_t current_freq = ledc_get_freq(channel_config->speed_mode, channel_config->timer_sel);
    
    esp_err_t ret = ESP_OK;

    // Check if the new frequency is different from the current one
    if (current_freq != freq_hz) {
        // Attempt to set the new frequency
        esp_err_t ret = ledc_set_freq(LEDC_MODE, LEDC_TIMER, freq_hz);
        if (ret != ESP_OK) {
            printf("Error setting frequency: %s\n", esp_err_to_name(ret));
        } else {
            printf("Frequency set successfully to: %lu Hz\n", freq_hz);
        }
    } else {
        printf("Current frequency matches the requested frequency: %lu Hz\n", freq_hz);
    }
    // 10-bit resolution
    uint32_t max_duty = (1 << LEDC_TIMER_10_BIT) - 1; // Assuming 10-bit resolution
    uint32_t duty = (max_duty * duty_percent) / 100;

    // Set the duty cycle 
    ret = ledc_set_duty(LEDC_MODE, pin_num, duty);
    if (ret != ESP_OK){
        printf("Error setting duty cycle: %s\n", esp_err_to_name(ret));
    } else {
        printf("Duty cycle set successfully to: %d%%\n", duty_percent);
    }

    // update duty cycle to take effect 
    ret = ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);
    if (ret != ESP_OK) { 
        printf("Error updating duty cycle: %s\n", esp_err_to_name(ret));
    } 
    // Replace the hardcoded CHANNEL_INDEX_TEST with the variable that you add in the CAN message
    if (ledc_channel_config(&ledc_channel_configs[PWM_CHANNEL_INDEX_IMD]) != ESP_OK) {
        printf("PWM_Channel_index_IMD_config failed\n");
    }
}
