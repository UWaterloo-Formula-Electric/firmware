#ifndef USER_TCP_H_
#define USER_TCP_H_

#define TCP_TASK_PERIOD_MS 5        /* TODO: needs to be tested. Andrew has no delay in his TCU code. 
                                     * His code can only run 1 task. */

/* TCP Configuration */
#define TCP_CLIENT_PORT 2333
#define DEST_IP_ADDR0	208
#define DEST_IP_ADDR1	68
#define DEST_IP_ADDR2	36
#define DEST_IP_ADDR3	87

#define DEST_PORT		2333
#define LOCAL_PORT		2333

#define MAX_DATA 50  // 最大数据项数

#define SERVER_IP "208.68.36.87" // bay服务器IP地址 - Owen's Server IP add, is set to forward to bay NUC, port 2333
#define SERVER_PORT 2333         // 服务器端口号 - Server port number

typedef struct {
    int id;
    char data[32];
    char str[32];  // 存储字符串形式
} DataItem;

void init_dataItems();
void add_or_update_data(int id, char data[]);
char* get_all_data_str();
char* format_hex(int id, int data);
                  
#endif /* USER_TCP_H_ */
