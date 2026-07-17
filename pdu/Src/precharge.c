#include "precharge.h"

void prechargeTask(void *pvParameters);
void Setup_PWM_Pin(uint8_t channel);
void Update_PWM_Duty_Cycle(uint8_t channel, uint8_t duty);
void Set_Pin_To_Standard_GPIO_High(uint8_t channel);


void prechargeTask(void *pvParameters) {
    PrechargeRequest_t req;
    // Precharge task implementation
    while (1) {
        // Wait for request to enter queue
        if (xQueueReceive(prechargeQueue, &req, portMAX_DELAY) == pdTRUE) {
            // Process the precharge request
            Setup_PWM_Pin(req.channel);

            float current_duty = 5.0;
            float step = 95/req.duration_ms; 
            while(current_duty < 100.0) {
                current_duty += step;
                if (current_duty > 100.0f) {  current_duty = 100.0f;}
                Update_PWM_Duty_Cycle(req.channel, (uint8_t)current_duty);
                vTaskDelay(pdMS_TO_TICKS(1)); // Delay for 1 millisecond
            }
            Set_Pin_To_Standard_GPIO_High(req.channel);
            vTaskDelay(pdMS_TO_TICKS(10)); 
        }

        // Delay for a specified period
        vTaskDelay(pdMS_TO_TICKS(1000)); // Delay for 1000 milliseconds
    }
}

static PrechargeHardware_t get_hardware_config(uint8_t channel) {
    PrechargeHardware_t config = {0};
    
    switch(channel) {
        case PRECHARGE_CHAN_1: // Logical Channel 1 (PC8 -> TIM3_CH3)
            config.gpio_port          = GPIOC;
            config.gpio_pin           = GPIO_PIN_8;
            config.tim_handle         = &htim3;
            config.tim_channel        = TIM_CHANNEL_3;
            config.alternate_function = GPIO_AF2_TIM3;
            break;

        case PRECHARGE_CHAN_2: // Logical Channel 2 (PC7 -> TIM3_CH2)
            config.gpio_port          = GPIOC;
            config.gpio_pin           = GPIO_PIN_7;
            config.tim_handle         = &htim3;
            config.tim_channel        = TIM_CHANNEL_2;
            config.alternate_function = GPIO_AF2_TIM3;
            break;

        case PRECHARGE_CHAN_3: // Logical Channel 3 (PD15 -> TIM4_CH4)
            config.gpio_port          = GPIOD;
            config.gpio_pin           = GPIO_PIN_15;
            config.tim_handle         = &htim4;
            config.tim_channel        = TIM_CHANNEL_4;
            config.alternate_function = GPIO_AF2_TIM4;
            break;

        case PRECHARGE_CHAN_4: // Logical Channel 4 (PD12 -> TIM4_CH1)
            config.gpio_port          = GPIOD;
            config.gpio_pin           = GPIO_PIN_12;
            config.tim_handle         = &htim4;
            config.tim_channel        = TIM_CHANNEL_1;
            config.alternate_function = GPIO_AF2_TIM4;
            break;

        default:
            // Safety fallback: return empty config
            break;
    }
    return config;
}

void Setup_PWM_Pin(uint8_t channel) {
    PrechargeHardware_t hw = get_hardware_config(channel);
    if (hw.gpio_port == NULL) return;

    // CubeMX already configured the GPIO alternate function.
    // We just ensure the pin mode is restored to Alternate Function (in case a previous run set it to standard GPIO)
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin       = hw.gpio_pin;
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull      = GPIO_NOPULL;
    GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = hw.alternate_function;
    HAL_GPIO_Init(hw.gpio_port, &GPIO_InitStruct);

    // Clear compare register and start PWM
    __HAL_TIM_SET_COMPARE(hw.tim_handle, hw.tim_channel, 0);
    HAL_TIM_PWM_Start(hw.tim_handle, hw.tim_channel);
}

