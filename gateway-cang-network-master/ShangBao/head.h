#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "MQTTClient.h"
#include "cJSON.h"
#include "shmem.h"
#include "msg_queue_peer.h"
#include <sys/stat.h>
#include <pthread.h>
#include <sqlite3.h>
#include <time.h>

// #define ADDRESS "tcp://192.168.50.80:1883"
#define CLIENTID "ExampleClient"
#define TOPIC_UP "/app/data/up"
#define TOPIC_DOWN "/app/data/down"
#define PAYLOAD "Hello World!"
#define QOS 1
#define TIMEOUT 10000L

char ADDRESS[32];
struct temp
{
    int tempkey;
    int temp1key;
};

union val_t {
    int b_val;   //bool类型存储空间
    int i_val;   //整形值存储空间
    float f_val; //浮点值存储空间
};

struct std_node
{
    int key;             //唯一键值
    int type;            //数据点类型
    int dev_type;        //数据点属于哪个设备，根据网关支持的设备自行定义---stm32->0 modbus->1
    union val_t old_val; //变化上报后需要更新旧值
    union val_t new_val; //从共享内存取出最新数据，放到new_val中
    int ret;             //默认为-1，采集成功后设置为0，采集失败再置-1
};

struct std_msg
{
    long msg_type; //消息类型
    int key;       //唯一键值
    char val[32];  //需要修改成的数据
};

void delivered(void *context, MQTTClient_deliveryToken dt);
int msgarrvd(void *context, char *topicName, int topicLen, MQTTClient_message *message);
void connlost(void *context, char *cause);
FILE *IOinit();
int parsejson(char *buff);
int parsemqttjson(char *buff);
int parsemqttjson(char *buff);
void *handler_thread(void *arg);
long long getCurrentSeconds();

struct temp tp;

