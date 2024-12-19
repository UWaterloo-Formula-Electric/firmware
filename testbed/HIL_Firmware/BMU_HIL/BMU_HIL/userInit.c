/***********************************
************ INCLUDES **************
************************************/

// Standard Includes
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ESP-IDF Includes
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "driver/twai.h"
#include "driver/spi_master.h"
#include "driver/rmt.h"
#include "driver/rmt_rx.h"
#include "hal/rmt_types.h"
#include "esp_log.h"

// Inter-Component Includes
#include "canReceive.h"

// Inta-Component Includes
#include "userInit.h"
#include "dac.h"

/***********************************
************ GLOBALS ***************
************************************/

spi_device_handle_t batt_pos;
spi_device_handle_t batt_neg;
spi_device_handle_t hv_neg_output;
spi_device_handle_t hv_pos_output;
spi_device_handle_t hv_shunt_pos;
spi_device_handle_t hv_shunt_neg;

// uses seperate isoSPI bus
spi_device_handle_t ams;

//Title of RMT log for fan PWM
static const char *TAG = "RMT_DUTY_CYCLE";
float fanDC = 0;

/***********************************
***** FUNCTION DEFINITIONS *********
************************************/ 

//UNTESTED: Using espressif documentation
esp_err_t RMT_Init(void){
    // RMT receiver configuration
    //Will be measuring duty cycle (BMU sends 0-100% at 88Hz as defined by datasheet)
    rmt_channel_handle_t rx_chan = NULL;
    rmt_crx_channel_config_t rx_chan_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,   // select source clock
        .resolution_hz = 1 * 1000 * 1000, // 1 MHz tick resolution, i.e., 1 tick = 1 µs
        .mem_block_symbols = 64,          // memory block size, 64 * 4 = 256 Bytes
        .gpio_num = FAN_PWM_PIN,          // GPIO number
        .flags.invert_in = false,         // do not invert input signal
        .flags.with_dma = false,          // do not need DMA backend
    };
    ESP_ERROR_CHECK(rmt_new_rx_channel(&rx_chan_config, &rx_chan));

    // Enable RMT receiver
    ESP_ERROR_CHECK(rmt_enable(rx_chan));

    QueueHandle_t receive_queue = xQueueCreate(1, sizeof(rmt_rx_done_event_data_t));
    rmt_rx_event_callbacks_t cbs = {
        .on_recv_done = example_rmt_rx_done_callback,
    };
    ESP_ERROR_CHECK(rmt_rx_register_event_callbacks(rx_channel, &cbs, receive_queue));

    // the following timing requirement is based on NEC protocol
    rmt_receive_config_t receive_config = {
        .signal_range_min_ns = 1250,     // the shortest duration for NEC signal is 560 µs, 1250 ns < 560 µs, valid signal is not treated as noise
        .signal_range_max_ns = 12000000, // the longest duration for NEC signal is 9000 µs, 12000000 ns > 9000 µs, the receive does not stop early
    };

    return ESP_OK;
}

//Callback for when receive is finished.
static bool example_rmt_rx_done_callback(rmt_channel_handle_t channel, const rmt_rx_done_event_data_t *edata, void *user_data)
{
    BaseType_t high_task_wakeup = pdFALSE;
    QueueHandle_t receive_queue = (QueueHandle_t)user_data;
    // send the received RMT symbols to the parser task
    xQueueSendFromISR(receive_queue, edata, &high_task_wakeup);
    // return whether any task is woken up
    return high_task_wakeup == pdTRUE;
}

//UNTESTED: From espressif documentation.
void readFanPWMTask(){
    printf("RMT Receive Task Running\r\n");
    //Make sure this task isn't starved, check-in
    TickType_t xLastWakeTime = xTaskGetTickCount();
    while(1){
         rmt_symbol_word_t raw_symbols[2]; // 64 symbols should be sufficient for a standard NEC frame
        //Stores 2 full RMT symbols which should be sufficient for duty cycle:
        //1 RMT symbol includes duration and level of one value, duration and level of opposite value.

        // ready to receive
        ESP_ERROR_CHECK(rmt_receive(rx_channel, raw_symbols, sizeof(raw_symbols), &receive_config));
        // parse the received symbols into
        example_parse_nec_frame(rx_data.received_symbols, rx_data.num_symbols);
        
        //Add caluclation of duty cycle here

        vTaskDelay(pdMS_TO_TICKS(500));  // Poll every 500 ms
    }
}

void taskRegister (void)
{
    BaseType_t xReturned = pdPASS;
    TaskHandle_t set_level_task_handler = NULL;
//     TaskHandle_t can_process_task_handler = NULL;


//  BaseType_t xTaskCreate( TaskFunction_t pvTaskCode,
//                          const char * const pcName,
//                          const configSTACK_DEPTH_TYPE uxStackDepth,
//                          void *pvParameters,
//                          UBaseType_t uxPriority,
//                          TaskHandle_t *pxCreatedTask
//                        );

//     //xTaskCreate desc: create task and add it to list of things ready to run
//     //TaskFunction_t taskCode, const char* const pcName, const configSTACK_DEPTH_TYPE 
//     //uxStackDepth, void* pvParameters, UBaseType_t uxPriority, TaskHandle pxCreatedTask

//     //Create the CAN Receive Task
//     xReturned = xTaskCreate(
//         can_rx_task,
//         "CAN_RECEIVE_TASK",
//         4000,
//         ( void * ) NULL,
//         configMAX_PRIORITIES-1,
//         &can_rx_task_handler
//     );

//     if(xReturned != pdPASS)
//     {
//         while(1)
//         {
//             printf("Failed to register can_rx_task to RTOS");
//         }
//     }

//Create fan PWM task to read fan PWM duty cycle every now and then
    xReturned = xTaskCreate(
        readFanPWMTask,
        "SET_LEVEL_TASK",
        4000,
        ( void * ) NULL,
        configMAX_PRIORITIES-1,
        &set_level_task_handler
    );

    if(xReturned != pdPASS)
    {
        while(1)
        {
            printf("Failed to register readFanPWMTask to RTOS");
        }
    }

//     //Create CAN process task
//     xReturned = xTaskCreate(
//         BMU_HIL_process_rx_task,
//         "HIL_CAN_PROCESS_TASK",
//         4000,
//         ( void * ) NULL,
//         configMAX_PRIORITIES-1,
//         &can_process_task_handler
//     );

//     if(xReturned != pdPASS)
//     {
//         while(1)
//         {
//             printf("Failed to register process_rx_task to RTOS");
//         }
//     }

    //TODO: Charger CAN Process and output
    //TODO: Output status
}


//UNTESTED
esp_err_t CAN_init (void)
{
//     memset(&rx_msg, 0, sizeof(twai_message_t));
//     memset(&can_msg, 0, sizeof(twai_message_t));
//     memset(&bmu_hil_queue, 0, sizeof(QueueHandle_t)); //not 0 ing other queues to avoid repetition

//     //TODO: these will probably have to be moved
//     twai_handle_t twai_hil;
//     twai_handle_t twai_chrgr;

//     // Config Structures
//     twai_general_config_t g_config = {
//         .mode = TWAI_MODE_NORMAL, 
//         .clkout_io = TWAI_IO_UNUSED, 
//         .bus_off_io = TWAI_IO_UNUSED,      
//         .tx_queue_len = MAX_CAN_MSG_QUEUE_LENGTH, 
//         .rx_queue_len = MAX_CAN_MSG_QUEUE_LENGTH,                          
//         .alerts_enabled = TWAI_ALERT_NONE,  
//         .clkout_divider = 0,        
//         .intr_flags = ESP_INTR_FLAG_LEVEL1
//     };
//     twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
//     twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

//     // Install CAN HIL driver
//     g_config.controller_id = CAN_HIL_ID,
//     g_config.tx_io = CAN_HIL_TX, 
//     g_config.rx_io = CAN_HIL_RX,
//     if (twai_driver_install_v2(&g_config, &t_config, &f_config, &twai_hil) != ESP_OK)
//     {
//         printf("Failed to install TWAI driver\r\n");
//         return ESP_FAIL;
//     } 
//     printf("TWAI driver installed\r\n");

//     // Start CAN HIL driver
//     if (twai_start_v2(&twai_hil) != ESP_OK) 
//     {
//         printf("Failed to start TWAI driver\r\n");
//         return ESP_FAIL;
        
//     } 
//     printf("TWAI driver started\r\n");

//     // Install CAN CHRGR driver
//     g_config.controller_id = CAN_CHRGR_ID,
//     g_config.tx_io = CAN_CHRGR_TX, 
//     g_config.rx_io = CAN_CHRGR_RX,
//     if (twai_driver_install_v2(&g_config, &t_config, &f_config, &twai_chrgr) != ESP_OK)
//     {
//         printf("Failed to install TWAI driver\r\n");
//         return ESP_FAIL;
//     } 
//     printf("TWAI driver installed\r\n");

//     // Start CAN CHRGR driver
//     if (twai_start_v2(&twai_chrgr) != ESP_OK) 
//     {
//         printf("Failed to start TWAI driver\r\n");
//         return ESP_FAIL;
        
//     } 
//     printf("TWAI driver started\r\n");
    return ESP_OK;
}

//UNTESTED
esp_err_t SPI_init(void)
{
    // printf("Initializing SPI bus\r\n");

    // esp_err_t ret = ESP_OK;

    // memset(&batt_pos, 0, sizeof(spi_device_handle_t));
    // memset(&batt_neg, 0, sizeof(spi_device_handle_t));
    // memset(&hv_neg_output, 0, sizeof(spi_device_handle_t));
    // memset(&hv_pos_output, 0, sizeof(spi_device_handle_t));
    // memset(&hv_shunt_pos, 0, sizeof(spi_device_handle_t));
    // memset(&hv_shunt_neg, 0, sizeof(spi_device_handle_t));

    // memset(&ams, 0, sizeof(spi_device_handle_t));

    // //Bus Configurations

    // spi_bus_config_t bus_config = {
    //     .mosi_io_num = SPI_MOSI_PIN,
    //     .sclk_io_num = SPI_CLK_PIN,
    //     .quadwp_io_num = NOT_USED,
    //     .quadhd_io_num = NOT_USED,
    // };

    // spi_bus_config_t isoBus_config = {
    //     .miso_io_num = ISOSPI_MISO_PIN,
    //     .mosi_io_num = ISOSPI_MOSI_PIN,
    //     .sclk_io_num = ISOSPI_CLK_PIN,
    //     .quadwp_io_num = NOT_USED,
    //     .quadhd_io_num = NOT_USED,
    // };

    // //Device Configurations

    // spi_device_interface_config_t battPos_config = {
    //     .clock_speed_hz = 20 * HZ_PER_MHZ,           //Clock out at 20 MHz
    //     .mode = 0,                                   //SPI mode 0
    //     .spics_io_num = BATT_POS_CS,                 //CS pin
    //     .queue_size = MAX_SPI_QUEUE_LENGTH,     
    // };

    // spi_device_interface_config_t battNeg_config = {
    //     .clock_speed_hz = 20 * HZ_PER_MHZ,           //Clock out at 20 MHz
    //     .mode = 0,                                   //SPI mode 0
    //     .spics_io_num = BATT_NEG_CS,                 //CS pin
    //     .queue_size = MAX_SPI_QUEUE_LENGTH,     
    // };

    // spi_device_interface_config_t hvNegOut_config = {
    //     .clock_speed_hz = 20 * HZ_PER_MHZ,           //Clock out at 20 MHz
    //     .mode = 0,                                   //SPI mode 0
    //     .spics_io_num = HV_NEG_OUT_CS,               //CS pin
    //     .queue_size = MAX_SPI_QUEUE_LENGTH,     
    // };

    // spi_device_interface_config_t hvPosOut_config = {
    //     .clock_speed_hz = 20 * HZ_PER_MHZ,           //Clock out at 20 MHz
    //     .mode = 0,                                   //SPI mode 0
    //     .spics_io_num = HV_POS_OUT_CS,               //CS pin
    //     .queue_size = MAX_SPI_QUEUE_LENGTH,     
    // };

    // spi_device_interface_config_t hvShuntPos_config = {
    //     .clock_speed_hz = 20 * HZ_PER_MHZ,           //Clock out at 20 MHz
    //     .mode = 0,                                   //SPI mode 0
    //     .spics_io_num = HV_SHUNT_POS_CS,             //CS pin
    //     .queue_size = MAX_SPI_QUEUE_LENGTH,     
    // };

    // spi_device_interface_config_t hvShuntNeg_config = {
    //     .clock_speed_hz = 20 * HZ_PER_MHZ,           //Clock out at 20 MHz
    //     .mode = 0,                                   //SPI mode 0
    //     .spics_io_num = HV_SHUNT_NEG_CS,             //CS pin
    //     .queue_size = MAX_SPI_QUEUE_LENGTH,     
    // };

    // //TODO: clock speed for AMS isoSPI
    // spi_device_interface_config_t ams_config = {
    //     .clock_speed_hz = 20 * HZ_PER_MHZ,           //Clock out at 20 MHz
    //     .mode = 0,                                   //SPI mode 0
    //     .spics_io_num = AMS_CS,                      //CS pin
    //     .queue_size = MAX_SPI_QUEUE_LENGTH,     
    // };

    // // Bus Initialization

    // ret = spi_bus_initialize(SPI2_HOST, &bus_config, SPI_DMA_CH_AUTO);
    // if(ret != ESP_OK){
    //     printf("failed to init SPI2 bus %d\r\n", ret);
    //     return ESP_FAIL;
    // }

    // ret = spi_bus_initialize(SPI3_HOST, &isoBus_config, SPI_DMA_CH_AUTO);
    // if(ret != ESP_OK){
    //     printf("failed to init SPI3 bus %d\r\n", ret);
    //     return ESP_FAIL;
    // }

    // // Adding Devices

    // ret = spi_bus_add_device(SPI2_HOST, &battPos_config, &batt_pos);
    // if(ret != ESP_OK){
    //     printf("failed to add throttle batt_pos to bus %d\r\n", ret);
    //     return ESP_FAIL;
    // }

    // ret = spi_bus_add_device(SPI2_HOST, &battNeg_config, &batt_neg);
    // if(ret != ESP_OK){
    //     printf("failed to add batt_neg to bus %d\r\n", ret);
    //     return ESP_FAIL;
    // }

    // ret = spi_bus_add_device(SPI2_HOST, &hvNegOut_config, &hv_neg_output);
    // if(ret != ESP_OK){
    //     printf("failed to add hv_neg_output to bus %d\r\n", ret);
    //     return ESP_FAIL;
    // }

    //     ret = spi_bus_add_device(SPI2_HOST, &hvPosOut_config, &hv_pos_output);
    // if(ret != ESP_OK){
    //     printf("failed to add hv_pos_output to bus %d\r\n", ret);
    //     return ESP_FAIL;
    // }

    // ret = spi_bus_add_device(SPI2_HOST, &hvShuntPos_config, &hv_shunt_pos);
    // if(ret != ESP_OK){
    //     printf("failed to add hv_shunt_pos to bus %d\r\n", ret);
    //     return ESP_FAIL;
    // }

    // ret = spi_bus_add_device(SPI2_HOST, &hvShuntNeg_config, &hv_shunt_neg);
    // if(ret != ESP_OK){
    //     printf("failed to add hv_shunt_neg to bus %d\r\n", ret);
    //     return ESP_FAIL;
    // }

    // ret = spi_bus_add_device(SPI3_HOST, &ams_config, &hv_shunt_neg);
    // if(ret != ESP_OK){
    //     printf("failed to add hv_shunt_neg to bus %d\r\n", ret);
    //     return ESP_FAIL;
    // }

    // TODO: init devices (see VCU HIL)

    return ESP_OK;
}

//UNTESTED: PWM Pins
esp_err_t PWM_init(void)
{
    //CONFIGURE PWM TIMERS
    ledc_timer_config_t fanTach_timer = {
        .speed_mode       = LEDC_LOW_SPEED_MODE, //Low Speed mode, only doing 60Hz
        .timer_num        = FAN_TACH_TIMER,      // Timer index
        .duty_resolution  = LEDC_TIMER_13_BIT, // Duty cycle resolution (2^13 levels)
        .freq_hz          = 0, // Frequency in Hz
        .clk_cfg          = LEDC_APB_CLK   // Using 80MHz reference tick
    };

    ledc_timer_config_t IMDStatus_timer = {
        .speed_mode       = LEDC_HIGH_SPEED_MODE, //Low Speed mode, only doing 60Hz
        .timer_num        = IMD_STATUS_TIMER,      // Timer index
        .duty_resolution  = LEDC_TIMER_13_BIT, // Duty cycle resolution (2^13 levels)
        .freq_hz          = IMD_STATUS_FREQ, // 9 to 55 MHz
        .clk_cfg          = LEDC_APB_CLK   // Using 80MHz reference tick
    };

    esp_err_t ret  =  led_timer_config(&fanTach_timer);
    if (ret != ESP_OK) {
        printf("fanTach timer configuration failed: %s\n", esp_err_to_name(ret));
        return ESP_FAIL;
    }

    ret  =  led_timer_config(&IMDStatus_timer);
    if (ret != ESP_OK) {
        printf("IMDStatus timer configuration failed: %s\n", esp_err_to_name(ret));
        return ESP_FAIL;
    }

    //CONFIGURE PWM CHANNELS
    ledc_channel_config_t fanTach_channel = {
        .gpio_num       = FAN_TACH_PIN,   // Output GPIO
        .speed_mode     = LEDC_LOW_SPEED_MODE, //Low speed mode
        .channel        = 0,    // LEDC Channel index
        .intr_type      = LEDC_INTR_DISABLE,
        .timer_sel      = FAN_TACH_TIMER,      // Use the configured timer
        .duty           = 50,      // Initial duty cycle and will keep constant DC (50%)
        //Unsure if highpoint needed. Time when output first latched ON
    };

    ledc_channel_config_t IMDStatus_channel = {
        .gpio_num       = IMD_STATUS_PIN,   // Output GPIO
        .speed_mode     = LEDC_HIGH_SPEED_MODE, //Low speed mode
        .channel        = 1,    // LEDC Channel index
        .intr_type      = LEDC_INTR_DISABLE,
        .timer_sel      = IMD_STATUS_TIMER,      // Use the configured timer
        .duty           = 0,      //Want to keep 0Hz duty cycle at the start
        //Unsure if highpoint needed. Time when output first latched ON
    };

    ret = ledc_channel_config(&fanTach_channel);
    if (ret != ESP_OK) {
        printf("fanTach channel configuration failed: %s\n", esp_err_to_name(ret));
        return ESP_FAIL;
    }

    ret = ledc_channel_config(&IMDStatus_channel);
    if (ret != ESP_OK) {
        printf("IMDStatus channel configuration failed: %s\n", esp_err_to_name(ret));
        return ESP_FAIL;
    }

    //SET INITIAL PWM VALUES
    ledc_set_freq(LEDC_LOW_SPEED_MODE, FAN_TACH_TIMER, 0); //Set FAN_TACH_PIN PWM to 0Hz frequency (fan is going 0 rpm)
    ledc_set_duty(LEDC_HIGH_SPEED_MODE, IMD_STATUS_TIMER, 0); //Set IMDStatus PWM to 0% duty cycle.
    ledc_update_duty(LEDC_HIGH_SPEED_MODE, IMD_STATUS_TIMER); //Update the duty cycle on the line, must be called after set

    return ESP_OK;
}

//TESTED: WORKS. GPIO Pins are able to be setup and initialized
esp_err_t GPIO_init (void)
{
    //SET DIRECTIONS
    //DCDC On
    gpio_set_direction(DCDC_ON_PIN, GPIO_MODE_INPUT);

    //Ampseal
    gpio_set_direction(CBRB_PRESS_PIN, GPIO_MODE_OUTPUT); 
    gpio_set_direction(IL_CLOSE_PIN, GPIO_MODE_OUTPUT); 
    gpio_set_direction(HVD_PIN, GPIO_MODE_OUTPUT); 
    gpio_set_direction(TSMS_FAULT_PIN, GPIO_MODE_OUTPUT); 

    //Contactor Control
    gpio_set_direction(CONT_NEG_PIN, GPIO_MODE_INPUT);
    gpio_set_direction(CONT_POS_PIN, GPIO_MODE_INPUT);

    //Reset Buttons
    gpio_set_direction(AMS_RESET_PRESS_PIN, GPIO_MODE_OUTPUT); 
    gpio_set_direction(IMD_RESET_PRESS_PIN, GPIO_MODE_OUTPUT); 
    gpio_set_direction(BPSD_RESET_PRESS_PIN, GPIO_MODE_OUTPUT); 

    //IMD
    gpio_set_direction(IMD_FAULT_PIN, GPIO_MODE_OUTPUT);

    //Fan: N/A
    
    //INITIALIZE OUTPUTS
    gpio_set_level(CBRB_PRESS_PIN,0);
    gpio_set_level(IL_CLOSE_PIN,0);
    gpio_set_level(HVD_PIN,0);
    gpio_set_level(TSMS_FAULT_PIN,0);

    gpio_set_level(AMS_RESET_PRESS_PIN,1);
    gpio_set_level(IMD_RESET_PRESS_PIN,1);
    gpio_set_level(BPSD_RESET_PRESS_PIN,1);

    gpio_set_level(IMD_FAULT_PIN,0); //Set to HIGH to signify fault. Using open drain, BMU recognizes 0V as a fault.

    return ESP_OK;
}

void app_main(void)
{
    // CAN_init();
    // SPI_init();
    PWM_Init();
    RMT_Init();
    GPIO_init();
    taskRegister();
}