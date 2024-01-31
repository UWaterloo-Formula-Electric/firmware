#include <stdio.h>

#include "pwm.h"

typedef enum 
{
    CHANNEL_INDEX_IMD = 0U,

    CHANNEL_INDEX_NUM // THIS HAS TO BE THE LAST ENTRY
} Channel_Index_E;

// Initialize LEDC for PWM
static ledc_channel_config_t ledc_channel_configs[] = {
    // TODO: Update name: CHANNEL_INDEX_TEST
    {
        .channel    = LEDC_CHANNEL,
        .duty       = 0, // Start with 0% duty cycle
        .gpio_num   = LEDC_GPIO,
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
// TODO consider no return value (void)
esp_err_t pwm_set_pin(uint8_t pin_num, uint32_t freq_hz, uint16_t duty_percent) {
    printf("setpwm\r\n");
    // TODO: Add channel to the PWM CAN message and process it in here to be configurable
    esp_err_t ret = ESP_OK;

        uint32_t current_freq = ledc_get_freq(LEDC_MODE, LEDC_TIMER);

    // Check if the new frequency is different from the current one
    if (current_freq != freq_hz) {
        // Attempt to set the new frequency
        ret = ledc_set_freq(LEDC_MODE, LEDC_TIMER, freq_hz);
        if (ret != ESP_OK) {
            printf("Error setting frequency: %s\n", esp_err_to_name(ret));
            return ret;
        }
    }
    else
    {
        printf("Setting pin frequency to: %lu\n", freq_hz);
    }

    // TODO: See if there is a max freq allowed

    // 10-bit resolution
    uint32_t duty = (duty_percent > 1023) ? 1023 : duty_percent;
    
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
    // Replace the hardcoded CHANNEL_INDEX_TEST with the variable that you add in the CAN message
    ret = ledc_channel_config(&ledc_channel_configs[CHANNEL_INDEX_IMD]);
    return ret;
}
