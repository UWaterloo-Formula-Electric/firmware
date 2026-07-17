typedef struct {
    uint8_t channel;
    uint16_t duration_ms;
} PrechargeRequest_t;

typedef struct {
    GPIO_TypeDef* gpio_port;
    uint16_t      gpio_pin;
    TIM_HandleTypeDef* tim_handle;
    uint32_t      tim_channel;
    uint8_t       alternate_function; // E.g., GPIO_AF2_TIM3 / GPIO_AF2_TIM4
} PrechargeHardware_t;

typedef enum {
    PRECHARGE_CHAN_1 = 0,  // Maps to PC8  -> TIM3_CH3
    PRECHARGE_CHAN_2,      // Maps to PC7  -> TIM3_CH2
    PRECHARGE_CHAN_3,      // Maps to PD15 -> TIM4_CH4
    PRECHARGE_CHAN_4,      // Maps to PD12 -> TIM4_CH1
    NUM_PRECHARGE_CHANNELS
} PrechargeChannel_t;

void prechargeTask(void *pvParameters);
void Setup_PWM_Pin(uint8_t channel);
void Update_PWM_Duty_Cycle(uint8_t channel, uint8_t duty);
void Set_Pin_To_Standard_GPIO_High(uint8_t channel);

QueueHandle_t prechargeQueue;

extern TIM_HandleTypeDef htim3;
extern TIM_HandleTypeDef htim4;

