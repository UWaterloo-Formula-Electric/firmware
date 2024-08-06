#include "user_tcp.h"

#include <string.h>

#include "uwfe_debug.h"

#include "lwip/netif.h"
#include "lwip/ip.h"
#include "lwip/tcp.h"
#include "lwip/init.h"
#include "netif/etharp.h"
#include "lwip/udp.h"
#include "lwip/pbuf.h"

#include "lwip/err.h"
#include "lwip/sockets.h"

#include "main.h"
#include "cmsis_os.h"

/* NOTE: Jacky commented out a lot of code that does nothing. 
 *  This code needs some serious clean up. This level of code should never be pushed
 *  to any repo. Code like this can never happen again.
 */

/**************************** Function Prototypes ****************************/
int create_tcp_client(void);
void tcp_client_send(int sock, const uint8_t *data, size_t dataLen);
void tcp_client_recv(int sock);
void TCP_Client_Init(void);
void send_json_data(struct tcp_pcb *tpcb, const char *json_data);

/***************************** Global Variables  *****************************/
struct tcp_pcb *client_pcb = NULL;
uint32_t err_times = 0;

DataItem dataItems[MAX_DATA];
// SemaphoreHandle_t dataMutex;

/**************************** Helper Functions ****************************/
// TODO: CHING CHONG LING LONG TING TONG
int create_tcp_client(void) {
    int sock;
    struct sockaddr_in server_address;
    int retry_count = 0;
    const int max_retries = 200;  // 设置最大重试次数
    // const int retry_delay = 2000;  // 每次重试之间的延时，单位：毫秒

    while (retry_count < max_retries) {
        // 创建socket
        sock = lwip_socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) {
            DEBUG_PRINT("soc create error...\n\r");
            return -1; // Socket创建失败
        }

        // 设置服务器地址
        server_address.sin_family = AF_INET;
        server_address.sin_port = htons(SERVER_PORT);
        inet_aton(SERVER_IP, &server_address.sin_addr.s_addr);

        // 连接服务器
        if (lwip_connect(sock, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
            lwip_close(sock);
            DEBUG_PRINT("connect error, retrying...\n\r");
            retry_count++;
//            osDelay(retry_delay);  // 延时后再重试
            vTaskDelay(pdMS_TO_TICKS(500));     // ??????
        } else {
            DEBUG_PRINT("Connected to server successfully after %d retries.\n\r",retry_count);
            return sock; // 成功连接，返回socket描述符
        }
    }

    DEBUG_PRINT("Failed to connect after max %d retries.\n\r", max_retries);
    return -2; // 多次重试后连接失败
}

void tcp_client_send(int sock, const uint8_t *data, size_t dataLen) {
	err_t err;
    err = lwip_send(sock, data, dataLen, 0);

	if (err == ERR_OK) {
//		tcp_output(tpcb);
		// 实际发送数据
	} else {
        DEBUG_PRINT("Failed to send data over TCP client\r\n");
//		DEBUG_PRINT("memerr..\n\r");
		// 处理错误
		// ERR_MEM 如果发送缓冲区太小不能放下全部数据
		// 其他错误码表示其他问题
	}
}

void tcp_client_recv(int sock) {
    char buffer[512];
    int len = lwip_recv(sock, buffer, sizeof(buffer), 0);
    if (len > 0) {
        // where's the rest of the code?
        // 处理接收到的数据
    }
}

static void client_err(void *arg, err_t err)       //出现错误时调用这个函数，打印错误信息，并尝试重新连接
{
//  DEBUG_PRINT("连接错误!!\n\r");
//	DEBUG_PRINT("尝试重连!!\n\r");

  //连接失败的时候释放TCP控制块的内存
	DEBUG_PRINT("TCP closed...\n\r");
    tcp_close(client_pcb);

  //重新连接
	DEBUG_PRINT("Attempting to reconnect...\n\r");
	TCP_Client_Init();
}

// static err_t client_send(void *arg, struct tcp_pcb *tpcb)   //发送函数，调用了tcp_write函数
// {
//   uint8_t send_buf[]= "Hi pc, this is STM32, How u doin????\n\r";

//   //发送数据到服务器
//   tcp_write(tpcb, send_buf, sizeof(send_buf), 1);

//   return ERR_OK;
// }

// static err_t client_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
// {
//   if (p != NULL)
//   {
//     /* 接收数据*/
//     tcp_recved(tpcb, p->tot_len);

//     /* 返回接收到的数据*/
//     tcp_write(tpcb, p->payload, p->tot_len, 1);

//     memset(p->payload, 0 , p->tot_len);
//     pbuf_free(p);
//   }
//   else if (err == ERR_OK)
//   {
//     //服务器断开连接
//     DEBUG_PRINT("Server disconnected!\n\r");
//     tcp_close(tpcb);

//     //重新连接
//     TCP_Client_Init();
//   }
//   return ERR_OK;
// }

static err_t client_connected(void *arg, struct tcp_pcb *pcb, err_t err)
{
    DEBUG_PRINT("Server connected!\n\r");

    // Fix this or remove it.
  //注册一个周期性回调函数
//  tcp_poll(pcb,client_send,2);

  //注册一个接收函数
//  tcp_recv(pcb,client_recv);

  return ERR_OK;
}

void TCP_Client_Init(void)
{
	//	struct tcp_pcb *client_pcb = NULL;   //这一句一定要放在里面，否则会没用
	client_pcb = NULL;   //这一句一定要放在里面，否则会没用
	ip4_addr_t server_ip;     //因为客户端要主动去连接服务器，所以要知道服务器的IP地址
	err_t err;
	/* 创建一个TCP控制块  */
	client_pcb = tcp_new();

	IP4_ADDR(&server_ip, DEST_IP_ADDR0,DEST_IP_ADDR1,DEST_IP_ADDR2,DEST_IP_ADDR3);//合并IP地址

	//  DEBUG_PRINT("客户端开始连接!\n\r");
	DEBUG_PRINT("StartConn...\n\r");
	//开始连接
	err = tcp_connect(client_pcb, &server_ip, TCP_CLIENT_PORT, client_connected);
	if(err != ERR_OK)
		DEBUG_PRINT("connErr!err=%d\n\r",err);
	err = ip_set_option(client_pcb, SOF_KEEPALIVE);
	DEBUG_PRINT("tcpconnCalled...\n\r");
	//	DEBUG_PRINT("已经调用了tcp_connect函数\n\r");

	//注册异常处理
	tcp_err(client_pcb, client_err);
	tcp_nagle_disable(client_pcb);
	//	DEBUG_PRINT("已经注册异常处理函数\n\r");
	DEBUG_PRINT("errFunRegisted.\n\r");
}

// 调用此函数来发送JSON数据
void send_json_data(struct tcp_pcb *tpcb, const char *json_data) {
    err_t err;

    // 将数据放入发送缓冲区
    err = tcp_write(tpcb, json_data, strlen(json_data), TCP_WRITE_FLAG_COPY);
    tcp_output(tpcb);
    if (err == ERR_OK) {
    	err_times = 0;
        // 实际发送数据
    } else {
    	DEBUG_PRINT("%lu merr,err=%i..\n\r",err_times,err);
    	err_times++;
        // 处理错误
        // ERR_MEM 如果发送缓冲区太小不能放下全部数据
        // 其他错误码表示其他问题
    }
}

//初始化存储空间
void init_dataItems() {
//    dataMutex = xSemaphoreCreateMutex();
    for (int i = 0; i < MAX_DATA; i++) {
        dataItems[i].id = -1;  // 使用-1表示该位置为空
        memset(dataItems[i].data, 0, sizeof(dataItems[i].data));
        memset(dataItems[i].str, 0, sizeof(dataItems[i].str));
    }
//    xSemaphoreGive(dataMutex);
}

//产生新数据
void add_or_update_data(int id, char data[]) {

//    xSemaphoreTake(dataMutex, portMAX_DELAY);
    int emptyIndex = -1;
//DEBUG_PRINT("running..\n");
    for (int i = 0; i < MAX_DATA; i++) {
        if (dataItems[i].id == id) {  // 找到相同ID，更新数据
        	 strncpy(dataItems[i].data, data, sizeof(dataItems[i].data) - 1);
        	 dataItems[i].data[sizeof(dataItems[i].data) - 1] = '\0'; // 确保字符串以null结尾
        	 sprintf(dataItems[i].str, ",%08lXx%08X%s\n", xTaskGetTickCount(), id, data);
//            xSemaphoreGive(dataMutex);
            return;
        }
        if (dataItems[i].id == -1 && emptyIndex == -1) {  // 记录第一个空位
            emptyIndex = i;
        }
    }

    if (emptyIndex != -1) {  // 有空位，添加新数据
        dataItems[emptyIndex].id = id;
        strncpy(dataItems[emptyIndex].data, data, sizeof(dataItems[emptyIndex].data) - 1);
        dataItems[emptyIndex].data[sizeof(dataItems[emptyIndex].data) - 1] = '\0'; // 确保字符串以null结尾
   	 	sprintf(dataItems[emptyIndex].str, ",%08lXx%08X%s\n", xTaskGetTickCount(), id, data);
    }

//    xSemaphoreGive(dataMutex);
}

char* get_all_data_str() {
    static char allDataStr[MAX_DATA * 29];  // 需要足够大的空间来存储所有字符串
//    xSemaphoreTake(dataMutex, portMAX_DELAY);
    strcpy(allDataStr, "");  // 初始化字符串

    for (int i = 0; i < MAX_DATA; i++) {
        if (dataItems[i].id != -1) {
            strcat(allDataStr, dataItems[i].str);
        }
    }
//    strcat(allDataStr, '\n');
//    DEBUG_PRINT(allDataStr);
//    xSemaphoreGive(dataMutex);
    init_dataItems();
    return allDataStr;
}

// update all CAN signals state to server
void update_CanLib_To_Server(){
        // ????
}


// Function to convert id and data into a formatted hex string
// General C
char* format_hex(int id, int data) {
    // Allocate memory for the resulting string
    static char result[29]; // Enough for ",0xAAAABBBB\0"
    // Format the id and data into the result string
    sprintf(result, ",0x%08X%016X", id, data);
    return result;
}
//example:
//char* formatted_string = format_hex(id, data);
//DEBUG_PRINT("%s\n", formatted_string);

/******************************* CAN CODE ******************************/

TickType_t failed = 0;
uint32_t failedNum = 0;

xQueueHandle CanMsgQueue;
typedef struct {
	uint32_t id;
	uint8_t data[8];
} CanMsg;
CanMsg fifoTmp;
volatile int32_t fifo0 = 0;
volatile int32_t fifo1 = 0;
char globalmsg[128];	//transfer msgs to string
int msg_len;

void configCANFilters2(CAN_HandleTypeDef* canHandle){
	CAN_FilterTypeDef  sFilterConfig;

	    // Filter msgs to this nodes Id to fifo 0
	    uint32_t filterID = 1<<8;//CAN_NODE_ADDRESS
	    filterID = filterID << 3; // Filter ID is left aligned to 32 bits
	    uint32_t filterMask = 0xFF00;
	    filterMask = filterMask << 3; // Filter masks are also left aligned to 32 bits
	    sFilterConfig.FilterMode = CAN_FILTERMODE_IDMASK;
	    sFilterConfig.FilterScale = CAN_FILTERSCALE_32BIT;
	    sFilterConfig.FilterIdHigh = (filterID>>16) & 0xFFFF;
	    sFilterConfig.FilterIdLow = (filterID & 0xFFFF);
	    sFilterConfig.FilterMaskIdHigh = (filterMask>>16) & 0xFFFF;
	    sFilterConfig.FilterMaskIdLow = (filterMask & 0xFFFF);
	    sFilterConfig.FilterFIFOAssignment = 0;
	    sFilterConfig.FilterActivation = ENABLE;
	    sFilterConfig.FilterBank = 0;

	    //From the reference manual, it seems that setting SlaveStartFilterBank to 0 means all filters are used for the enabled CAN peripheral
	    //TODO: Verify this is the correct config
	    sFilterConfig.SlaveStartFilterBank = 0;
	    if(HAL_CAN_ConfigFilter(canHandle, &sFilterConfig) != HAL_OK)
	    {
	        Error_Handler();
	    }

	    // Filter msgs to the broadcast Id to fifo 0
	    filterID = 0xFF<<8;
	    filterID = filterID << 3; // Filter ID is left aligned to 32 bits
	    filterMask = 0xFF00;
	    filterMask = filterMask << 3; // Filter masks are also left aligned to 32 bits
	    sFilterConfig.FilterMode = CAN_FILTERMODE_IDMASK;
	    sFilterConfig.FilterScale = CAN_FILTERSCALE_32BIT;
	    sFilterConfig.FilterIdHigh = (filterID>>16) & 0xFFFF;
	    sFilterConfig.FilterIdLow = (filterID & 0xFFFF);
	    sFilterConfig.FilterMaskIdHigh = (filterMask>>16) & 0xFFFF;
	    sFilterConfig.FilterMaskIdLow = (filterMask & 0xFFFF);


	    sFilterConfig.FilterFIFOAssignment = 0;
	    sFilterConfig.FilterActivation = ENABLE;
	    sFilterConfig.FilterBank = 1;

	    //From the reference manual, it seems that setting SlaveStartFilterBank to 0 means all filters are used for the enabled CAN peripheral
	    //TODO: Verify this is the correct config
	    sFilterConfig.SlaveStartFilterBank = 0;
	    if(HAL_CAN_ConfigFilter(canHandle, &sFilterConfig) != HAL_OK)
	    {
	        Error_Handler();
	    }

	    // Filter msgs to the broadcast Id to fifo 0
	    filterID = 1<<12;
	    filterID = filterID << 3; // Filter ID is left aligned to 32 bits
	    filterMask = 0xFF00;
	    filterMask = filterMask << 3; // Filter masks are also left aligned to 32 bits
	    sFilterConfig.FilterMode = CAN_FILTERMODE_IDMASK;
	    sFilterConfig.FilterScale = CAN_FILTERSCALE_32BIT;
	    sFilterConfig.FilterIdHigh = (filterID>>16) & 0xFFFF;
	    sFilterConfig.FilterIdLow = (filterID & 0xFFFF);
	    sFilterConfig.FilterMaskIdHigh = (filterMask>>16) & 0xFFFF;
	    sFilterConfig.FilterMaskIdLow = (filterMask & 0xFFFF);
	    sFilterConfig.FilterFIFOAssignment = 0;
	    sFilterConfig.FilterActivation = ENABLE;
	    sFilterConfig.FilterBank = 3;

	    sFilterConfig.SlaveStartFilterBank = 0;

	    //From the reference manual, it seems that setting SlaveStartFilterBank to 0 means all filters are used for the enabled CAN peripheral
	    //TODO: Verify this is the correct config
	    if(HAL_CAN_ConfigFilter(canHandle, &sFilterConfig) != HAL_OK)
	    {
	        Error_Handler();
	    }
}

void configCANFilters(CAN_HandleTypeDef* canHandle){	//Without filtered any CAN msgs

    CAN_FilterTypeDef sFilterConfig;

    // Configure the filter to accept all messages
    sFilterConfig.FilterMode = CAN_FILTERMODE_IDMASK;
    sFilterConfig.FilterScale = CAN_FILTERSCALE_32BIT;
    sFilterConfig.FilterIdHigh = 0x0000;
    sFilterConfig.FilterIdLow = 0x0000;
    sFilterConfig.FilterMaskIdHigh = 0x0000;
    sFilterConfig.FilterMaskIdLow = 0x0000;
    sFilterConfig.FilterFIFOAssignment = 0;
    sFilterConfig.FilterActivation = ENABLE;
    sFilterConfig.FilterBank = 0;  // Use filter bank 0

    // From the reference manual, it seems that setting SlaveStartFilterBank to 0 means all filters are used for the enabled CAN peripheral
    // TODO: Verify this is the correct config
    sFilterConfig.SlaveStartFilterBank = 0;

    if (HAL_CAN_ConfigFilter(canHandle, &sFilterConfig) != HAL_OK)
    {
        Error_Handler();
    }
}

void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
	fifo0++;
    CAN_RxHeaderTypeDef   RxHeader;
    uint8_t               RxData[8];

    if (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &RxHeader, RxData) == HAL_OK)
    {
    	fifo0++;
    	fifo1++;
      fifoTmp.id = RxHeader.ExtId;
      memcpy(fifoTmp.data, RxData, 8*sizeof(uint8_t));
      if (xQueueSendToBackFromISR(CanMsgQueue, &fifoTmp, NULL) != pdTRUE)
      {
        if (failed == 0)
        {
          failed = xTaskGetTickCountFromISR();
          failedNum =fifo0;
        }
      }
    }
    else {
        uint8_t msg[] = "Failed to receive CAN message from FIFO0\n";
        HAL_UART_Transmit(&huart1, msg, sizeof(msg), 100);
        // handleError();
    }

    /*
        This check is essential as it was causing issues with our brake light flashing and our button presses were getting random values.
        The cause is the motor controllers who send messages with standard ID lengths. So since we are passing in RxHeader.ExtId
        the data that was being received changed but the id did not so it would call the callback of the last extended message that was processed.
        This is why we would get brake light values of 255 and button presses with multiple bits high even though that is impossible from our code.
        Props to Joseph Borromeo for squashing this 5 year old bug
    */
    // if (RxHeader.IDE == CAN_ID_EXT){  // Only parse data if it is an extended CAN frame
    //         // if (parseCANData(RxHeader.ExtId, RxData) != HAL_OK) {
    //         //     /*ERROR_PRINT_ISR("Failed to parse CAN message id 0x%lX", RxHeader.ExtId);*/
    //         // }
    // }
}

//Currently not used (we use FIFO0)
void HAL_CAN_RxFifo1MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
	fifo1++;
    CAN_RxHeaderTypeDef   RxHeader;
    uint8_t               RxData[8];

    if (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO1, &RxHeader, RxData) != HAL_OK)
    {
        uint8_t msg[] = "Failed to receive CAN message from FIFO1\n";
        HAL_UART_Transmit(&huart1, msg, sizeof(msg), 100);
        // handleError();
    }

    // if (RxHeader.IDE == CAN_ID_EXT){  // Only parse data if it is an extended CAN frame
    //         if (parseCANData(RxHeader.ExtId, RxData) != HAL_OK) {
    //             /*ERROR_PRINT_ISR("Failed to parse CAN message id 0x%lX", RxHeader.ExtId);*/
    //         }
    // }
}

/**************************** FreeRTOS Task ****************************/
void tcpTaskFunction(void *pvParameters)
{
    uint32_t dbwTaskNotifications;

    TickType_t xLastWakeTime = xTaskGetTickCount();

    // Jacky: Wait for LWIP initialization that is done in mainTask (can't be moved)
    if (xTaskNotifyWait(0x00, UINT32_MAX, &dbwTaskNotifications, portMAX_DELAY) != pdPASS)
    {
        DEBUG_PRINT("Failed to receive notification from the default task in TCU!\r\n");
    }
    
    CanMsgQueue = xQueueCreate(4000, sizeof(CanMsg)); // Good to 72%
    if( CanMsgQueue == NULL )
    {
    //	  while(1)
    //	  {
            uint8_t msg[] = "Failed to create CAN queue\r\n";
            HAL_UART_Transmit(&huart1, msg, sizeof(msg), 100);
            vTaskDelay(pdMS_TO_TICKS(1000));
            HAL_UART_Transmit(&huart1, msg, sizeof(msg), 100);
    //	  }
    }

    int tcp_socket = create_tcp_client();

    
    // Enable CAN reception after successful socket connection
    DEBUG_PRINT("enabling CAN reception\r\n");
    if (HAL_CAN_Start(&hcan1) != HAL_OK) {
        uint8_t msg[] = "Failed to start CAN!\n";
        HAL_UART_Transmit(&huart1, msg, sizeof(msg), 100);
        return;
    }

    if (HAL_CAN_ActivateNotification(&hcan1, CAN_IT_RX_FIFO0_MSG_PENDING) != HAL_OK) {
	  uint8_t msg[] = "Error starting to listen for CAN msgs from FIFO0\n";
	  HAL_UART_Transmit(&huart1, msg, sizeof(msg), 100);
	  return;
    }

    if (HAL_CAN_ActivateNotification(&hcan1, CAN_IT_RX_FIFO1_MSG_PENDING) != HAL_OK) {
        uint8_t msg[] = "Error starting to listen for CAN msgs from FIFO1\n";
        HAL_UART_Transmit(&huart1, msg, sizeof(msg), 100);
        return;
    }

    CanMsg rxMsg;
    const uint16_t buffSize = 3000;
    uint8_t buffer[buffSize];
    memset(buffer, 0, buffSize*sizeof(uint8_t));
    const uint8_t logging_line_len = 35;//with comma

    while (1)
    {
        uint32_t bufferFilled = 0U;
        for (uint16_t loop = 0; loop < buffSize/logging_line_len; loop++)
        {
            if (xQueueReceive(CanMsgQueue, &rxMsg, 100) == pdTRUE)
            {
                sprintf((char *)&buffer[loop*logging_line_len], ",%08lXx%08lX%02X%02X%02X%02X%02X%02X%02X%02X\n", xTaskGetTickCount(), rxMsg.id, rxMsg.data[0], rxMsg.data[1], rxMsg.data[2], rxMsg.data[3], rxMsg.data[4], rxMsg.data[5], rxMsg.data[6], rxMsg.data[7]);
                fifo1--;
                bufferFilled += logging_line_len;
            }
            else
            {
                DEBUG_PRINT("Q Empty\n");
                break;
            }
        }
        if (bufferFilled != 0)
        {
            size_t totalLength = (size_t)bufferFilled;
            tcp_client_send(tcp_socket, buffer, totalLength);
        }
        if (failed != 0)
        {
            DEBUG_PRINT("failed: %lu %lu\r\n", failed, failedNum);
        }

    	HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_0);  // what? This pin is not even initialized?
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(TCP_TASK_PERIOD_MS));
    }
}