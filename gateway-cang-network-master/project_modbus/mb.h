#ifndef MB_H
#define MB_H

// #define SERVER_IP "192.168.50.208"  // Modbus服务器的IP地址
// #define SERVER_PORT 502             // Modbus服务器的端口号
#define SERVER_ID 1                 // Modbus从机ID

#define MSG_QUEUE_NAME "msg"

#define SHM_NAME "shm"
#define SHM_SIZE 1024

#include "list.h"
#include "shmem.h"
#include <modbus.h>
#include <pthread.h>
#include "msg_queue_peer.h"
#include "shmem.h"

// 定义modbus ip&port
char SERVER_IP[32];
int SERVER_PORT;
// 定义 Modbus 数据结构
struct mb_node {
    int key;           // 唯一键值
    char name[128];    // 数据点名称
    int type;          // 数据点类型
    int addr;          // 数据点地址
};

// 定义链表节点结构
struct mb_node_list {
    struct mb_node node;
    struct list_head list;  // 内核链表节点
};

// 共享内存格式
union val_t
{ 
    int b_val;  //bool类型存储空间
    int i_val;   //整形值存储空间
    float f_val;  //浮点值存储空间
};

struct std_node
{
	int key;  //唯一键值
	int type;  //数据点类型
    int dev_type;  //数据点属于哪个设备，根据网关支持的设备自行定义
	union val_t old_val;  //变化上报后需要更新旧值
    union val_t new_val;  //从共享内存取出最新数据，放到new_val中
    int ret;  //默认为-1，采集成功后设置为0，采集失败再置-1
};

//创建用于接受消息队列的结构体
//消息队列
struct std_msg
{
    long msg_type;  //消息类型
    int key;        //唯一键值
    char val[32];  //需要修改成的数据
};

// 从 JSON 文件中解析 modbus 数据并插入链表
void parse_and_insert_modbus_data(const char *json_str, struct list_head *mb_list);

#endif // MB_H