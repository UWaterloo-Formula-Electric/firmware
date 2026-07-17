typedef struct {
    uint8_t channel;
    uint16_t duration_ms;
} PrechargeRequest_t;

QueueHandle_t prechargeQueue;