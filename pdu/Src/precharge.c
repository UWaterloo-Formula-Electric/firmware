#include "precharge.h"



void prechargeTask(void *pvParameters) {
    PrechargeRequest_t req;
    // Precharge task implementation
    while (1) {
        // Wait for request to enter queue
        if (xQueueReceive(prechargeQueue, &req, portMAX_DELAY) == pdTRUE) {
            // Process the precharge request
            Setup_PWM_Pin(req.channel);

            float current_duty = 0.05;
            float step = 0.95/req.duration_ms; 
            while(current_duty < 1.0) {
                current_duty += step;
                if (current_duty > 1.0f) {  current_duty = 1.0f;}
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
        case BMU_Channel: // Logical Channel 1 (PC8 -> TIM3_CH3)
            config.gpio_port          = GPIOC;
            config.gpio_pin           = GPIO_PIN_8;
            config.tim_handle         = &htim3;
            config.tim_channel        = TIM_CHANNEL_3;
            config.alternate_function = GPIO_AF2_TIM3;
            break;

        case CDU_Channel: // Logical Channel 2 (PC7 -> TIM3_CH2)
            config.gpio_port          = GPIOC;
            config.gpio_pin           = GPIO_PIN_7;
            config.tim_handle         = &htim3;
            config.tim_channel        = TIM_CHANNEL_2;
            config.alternate_function = GPIO_AF2_TIM3;
            break;

        case TCU_Channel: // Logical Channel 3 (PD15 -> TIM4_CH4)
            config.gpio_port          = GPIOD;
            config.gpio_pin           = GPIO_PIN_15;
            config.tim_handle         = &htim4;
            config.tim_channel        = TIM_CHANNEL_4;
            config.alternate_function = GPIO_AF2_TIM4;
            break;

        case INV_Channel: // Logical Channel 4 (PD12 -> TIM4_CH1)
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

void Update_PWM_Duty_Cycle(uint8_t channel, uint8_t duty);
{
    
    PrechargeHardware_t hw = get_hardware_config(channel);
    if (hw.gpio_port == NULL) return;

    // Calculate the compare value based on duty cycle percentage
    uint32_t period = __HAL_TIM_GET_AUTORELOAD(hw.tim_handle);
    uint32_t compare_value = (period + 1) * duty; // +1 because period is zero-based

    // Update the compare register to change the duty cycle
    __HAL_TIM_SET_COMPARE(hw.tim_handle, hw.tim_channel, compare_value);
}

void Set_Pin_To_Standard_GPIO_High(uint8_t channel){
    PrechargeHardware_t hw = get_hardware_config(channel);
    if (hw.gpio_port == NULL) return;

    // Force compare register to exceed or equal the Auto-Reload Register (ARR)
    // This forces the PWM output to stay constantly active (100% duty)
    uint32_t period = __HAL_TIM_GET_AUTORELOAD(hw.tim_handle);
    __HAL_TIM_SET_COMPARE(hw.tim_handle, hw.tim_channel, period);
}