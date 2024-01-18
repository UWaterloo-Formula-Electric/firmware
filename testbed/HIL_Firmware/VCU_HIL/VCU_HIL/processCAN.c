#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/twai.h"
#include "userInit.h"
#include "dac.h"
#include "canReceive.h"
#include "processCAN.h"

static uint16_t dbyte1;
static uint16_t dbyte2;

//reassemble data into a 16 bit uint to pass in desired voltage into set dac function
static uint16_t dbyte2_mask = 0x00FF;
static uint16_t dbyte1_mask = 0xFF00;



#define LEDC_CHANNEL    LEDC_CHANNEL_0
#define LEDC_TIMER      LEDC_TIMER_0
#define LEDC_MODE       LEDC_LOW_SPEED_MODE
#define LEDC_GPIO       (5) // Replace with the specific GPIO number you want to use
#define LEDC_FREQUENCY  (10000) // Frequency in Hz, adjust as needed for your tests

// Function to initialize LEDC for PWM
static void ledc_init() {
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

    // Initialize the LEDC timer and channel
    ledc_timer_config(&ledc_timer);
    ledc_channel_config(&ledc_channel);
}

// Function to set the PWM duty cycle
static esp_err_t set_pwm_duty(uint32_t duty_percent) {
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

// Function to set the PWM frequency
static esp_err_t set_pwm_freq(uint32_t freq_mhz) {
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
static void simulate_imd_status(uint32_t freq_mhz, uint32_t duty_percent) {
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


void process_rx_task (void * pvParameters)
{
    TickType_t xLastWakeTime = xTaskGetTickCount();
        // Initialize LEDC for PWM
    ledc_init();
    // Example: Set duty cycle to 50% for testing
    set_pwm_duty(50);

    while(1)
    {
        // Example simulation: IMDSTATUS_Undervoltage
        // Frequency: 19000 to 21000 mHz, Duty Cycle: 5% to 95%
        simulate_imd_status(20000, 50);
        // Add more simulations as needed based on your test plan
        // Example simulation: IMDSTATUS_Normal
        // Frequency: 9500 to 10500 mHz, Duty Cycle: 5% to 84%
        simulate_imd_status(10000, 40);
    // Continue to simulate other conditions as required
    // {
    //     xQueueReceive(vcu_hil_queue, &can_msg, portMAX_DELAY);

    //     switch (can_msg.identifier)
    //     {
    //         case BRAKE_POS:     //Brake position
    //             dbyte1 = can_msg.data[0];
    //             dbyte2 = can_msg.data[1];
    //             dbyte2 = dbyte2 << 8;
    //             dbyte2 |= dbyte2_mask;
    //             dbyte1 |= dbyte1_mask;
    //             dbyte2 &= dbyte1;
    //             set6551Voltage(dbyte2, brakePos_ID);


    
    //             break;
    //         case BRAKE_PRES_RAW:     //Brake pres raw
    //             dbyte1 = can_msg.data[0];
    //             dbyte2 = can_msg.data[1];
    //             dbyte2 = dbyte2 << 8;
    //             dbyte2 |= dbyte2_mask;
    //             dbyte1 |= dbyte1_mask;
    //             dbyte2 &= dbyte1;
    //             setDacVoltage(dbyte2);
    //             break;
    //         case THROTTLE_A:     //Throttle A 
    //             dbyte1 = can_msg.data[0];
    //             dbyte2 = can_msg.data[1];
    //             dbyte2 = dbyte2 << 8;
    //             dbyte2 |= dbyte2_mask;
    //             dbyte1 |= dbyte1_mask;
    //             dbyte2 &= dbyte1;
    //             set6551Voltage(dbyte2, throttleA_ID);
    //             break;
    //         case THROTTLE_B:     //Throttle B
    //             dbyte1 = can_msg.data[0];
    //             dbyte2 = can_msg.data[1];
    //             dbyte2 = dbyte2 << 8;
    //             dbyte2 |= dbyte2_mask;
    //             dbyte1 |= dbyte1_mask;
    //             dbyte2 &= dbyte1;
    //             set6551Voltage(dbyte2, throttleB_ID);
    //             break;
    //         case STEER_RAW:     //Steer Raw
    //             dbyte1 = can_msg.data[0];
    //             dbyte2 = can_msg.data[1];
    //             dbyte2 = dbyte2 << 8;
    //             dbyte2 |= dbyte2_mask;
    //             dbyte1 |=dbyte1_mask;
    //             dbyte2 &= dbyte1;
                    IMD_PIN = 5
                    pwm_frew

                    pwm_duty
    //             set6551Voltage(dbyte2, steerRaw_ID);
    //             break;

    //         default:
    //             break;
        // }

        vTaskDelayUntil(&xLastWakeTime, PROCESS_RX_TASK_INTERVAL);
    }
}