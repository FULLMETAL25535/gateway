/***********************************************************************************
Copy right:	    hqyj Tech.
Author:         jiaoyue
Date:           2023.07.01
Description:    http请求处理
***********************************************************************************/

#include <sys/types.h>
#include <sys/socket.h>
#include "custom_handle.h"
#include <stdio.h>
#include <pthread.h>
#include <modbus.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <errno.h>
#include <string.h>
#include "msg_queue_peer.h"
#include "shmem.h"

#define KB 1024
#define HTML_SIZE (64 * KB)
// int findKeyByName(const char *jsonString, const char *name);
// int readJsonFromFile(const char *filename, char **jsonString);
//新添加的共享内存结构体
union val_t {
    int b_val;   //bool类型存储空间
    int i_val;   //整形值存储空间
    float f_val; //浮点值存储空间
};
int extractKeyFromName(const char *name, cJSON *json); //声明
struct std_node
{
    int key;             //唯一键值
    int type;            //数据点类型
    int dev_type;        //数据点属于哪个设备，根据网关支持的设备自行定义
    union val_t old_val; //变化上报后需要更新旧值
    union val_t new_val; //从共享内存取出最新数据，放到new_val中
    int ret;             //默认为-1，采集成功后设置为0，采集失败再置-1
};
#define BUFFER_SIZE 2512                                              //存取点表读取的数据大小
static struct shm_param para;                                         //
void create_json(struct std_node *addr, char *json_buffer, int *num); //声明转为JSON函数
//新添加的消息队列结构体
// union val_t
// {
//     int b_val;  //bool类型存储空间，控制扫地机器人，窗帘
//     int i_val;   //整形值存储空间
//     float f_val;  //浮点值存储空间
// };

// 消息队列
struct std_msg
{
    long msg_type; //消息类型
    int key;       //唯一键值
    char val[32];  //需要修改成的数据
};

//以上新添加的消息队列结构体
struct msg //共享空间结构体
{
    int flag;
    char buf[32];
};

struct msgbuf //消息队列结构体
{
    long type; //必须用long，表示消息的类型，值>0
    //正文随便定义
    int a;
    int b;
};

//普通的文本回复需要增加html头部
#define HTML_HEAD "Content-Type: text/html\r\n" \
                  "Connection: close\r\n"

static int handle_login(int sock, const char *input)
{
    char reply_buf[HTML_SIZE] = {0};
    char *uname = strstr(input, "username=");
    uname += strlen("username=");
    char *p = strstr(input, "password");
    *(p - 1) = '\0';
    printf("username = %s\n", uname);

    char *passwd = p + strlen("password=");
    printf("passwd = %s\n", passwd);

    if (strcmp(uname, "admin") == 0 && strcmp(passwd, "admin") == 0)
    {
        sprintf(reply_buf, "<script>localStorage.setItem('usr_user_name', '%s');</script>", uname);
        strcat(reply_buf, "<script>window.location.href = '/index4.html';</script>");
        send(sock, reply_buf, strlen(reply_buf), 0);
    }
    else
    {
        printf("web login failed\n");

        //"用户名或密码错误"提示，chrome浏览器直接输送utf-8字符流乱码，没有找到太好解决方案，先过渡
        char out[128] = {0xd3, 0xc3, 0xbb, 0xa7, 0xc3, 0xfb, 0xbb, 0xf2, 0xc3, 0xdc, 0xc2, 0xeb, 0xb4, 0xed, 0xce, 0xf3};
        sprintf(reply_buf, "<script charset='gb2312'>alert('%s');</script>", out);
        strcat(reply_buf, "<script>window.location.href = '/login.html';</script>");
        send(sock, reply_buf, strlen(reply_buf), 0);
    }

    return 0;
}

static int handle_add(int sock, const char *input)
{
    int number1, number2;

    //input必须是"data1=1data2=6"类似的格式，注意前端过来的字符串会有双引号
    sscanf(input, "\"data1=%ddata2=%d\"", &number1, &number2);
    printf("num1 = %d\n", number1);

    char reply_buf[HTML_SIZE] = {0};
    printf("num = %d\n", number1 * number2);
    sprintf(reply_buf, "%d", number1 * number2);
    printf("resp = %s\n", reply_buf);
    send(sock, reply_buf, strlen(reply_buf), 0);

    return 0;
}

static int handle_get(int sock, const char *input)
{
    // struct msg *p;

    // struct shm_param para;

    // if (shm_init(&para, "test", 1024) < 0)
    // {
    //     printf("shm init err\n");
    //     return -1;
    // }

    // p = shm_getaddr(&para);
    // if (p == NULL)
    // {
    //     printf("shm get addr err\n");
    //     return -1;
    // }

    // //操作共享内存
    // char buf_s[32];
    // printf("buf=%s", p->buf);
    // sprintf(buf_s, "%s", p->buf);
    // printf("buf_s=%s\n", buf_s);
    // send(sock, buf_s, strlen(buf_s), 0);
}
static int handle_get2(int sock, const char *input)
{
    char versionString[100];
    struct utsname uts;
    if (uname(&uts) == 0)
    {
        // uts.release 中通常包含内核版本信息

        strcpy(versionString, uts.release);
        printf("虚拟机主机版本: %s\n", versionString);
    }
    else
    {
        perror("获取系统信息失败");
        return 1;
    }

    send(sock, versionString, strlen(versionString), 0);
}

static int handle_get3(int sock, const char *input)
{
    // 打开数据库，然后关闭
    sqlite3 *db = NULL;
    int ret = sqlite3_open("/home/hq/24022/Bigger/gateway-cang-network/ShangBao/history.db", &db);

    if (ret!= SQLITE_OK) {
        printf("open err");
        return -1;
    }

    char *errmsg;
    int n_row, n_cloum;

    // 构建查询语句获取最后 20 个 temp 和 temp1 的值
    char sql[256];
    sprintf(sql, "SELECT temp, temp1 FROM history ORDER BY rowid DESC LIMIT 60");

    sqlite3_stmt *stmt;
    ret = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);

    if (ret!= SQLITE_OK) {
        printf("Prepare error: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return -1;
    }

    float data[60][2];  // 用于存储 temp 和 temp1 的值
    int count = 0;

    while ((ret = sqlite3_step(stmt)) == SQLITE_ROW) {
        data[count][0] = sqlite3_column_double(stmt, 0);
        data[count][1] = sqlite3_column_double(stmt, 1);
        count++;
    }

    sqlite3_finalize(stmt);

    // 将数组转换为 JSON 字符串
    cJSON *root = cJSON_CreateObject();
    cJSON *arr = cJSON_CreateArray();

    for (int i = 0; i < count; i++) {
        cJSON *item = cJSON_CreateObject();
        cJSON_AddNumberToObject(item, "temp", data[i][0]);
        cJSON_AddNumberToObject(item, "temp1", data[i][1]);
        cJSON_AddItemToArray(arr, item);
    }

    cJSON_AddItemToObject(root, "data", arr);

    char *jsonString = cJSON_PrintUnformatted(root);

    // 发送 JSON 字符串给网页端
    // 假设 sock 已经正确初始化，这里只是模拟发送
    printf("Sending JSON: %s\n", jsonString);
    send(sock, jsonString, strlen(jsonString), 0);
    cJSON_Delete(root);
    free(jsonString);

    sqlite3_close(db);

    
}
static int handle_get4(int sock, const char *input)
{
    //这里的input是json格式的请求
        printf("input = %s\n", input);
        //主线程循环读入共享内存
        int ret = -1;
        ret = shm_init(&para, "shm", 1024);
        if (ret < 0)
        {
            return -1;
        }
        int *num = shm_getaddr(&para);
        //模拟
        // *num = 5;
        //
        if (num == NULL)
        {
            return -1;
        }
        struct std_node *addr = (struct std_node *)(num + 1);
        printf("%d\n", *num);
        printf("%d\n", addr->key);
        // 模拟从共享内存中获取到的数据
        // addr->key = 101;
        // addr->type = 1;
        // addr->dev_type = 2;
        // addr->old_val.f_val = 10.5;
        // addr->new_val.i_val = 20;
        // addr->ret = 0;
        // //
        // (addr + 1)->key = 102;
        // (addr + 1)->type = 1;
        // (addr + 1)->dev_type = 2;
        // (addr + 1)->old_val.f_val = 10.5;
        // (addr + 1)->new_val.i_val = 22.0;
        // (addr + 1)->ret = 0;
        // //
        // (addr + 2)->key = 306;
        // (addr + 2)->type = 1;
        // (addr + 2)->dev_type = 2;
        // (addr + 2)->old_val.f_val = 10.5;
        // (addr + 2)->new_val.i_val = 38;
        // (addr + 2)->ret = 0;
        // //
        // (addr + 3)->key = 304;
        // (addr + 3)->type = 1;
        // (addr + 3)->dev_type = 2;
        // (addr + 3)->old_val.f_val = 10.5;
        // (addr + 3)->new_val.i_val = 55;
        // (addr + 3)->ret = 0;
        // //
        // (addr + 4)->key = 305;
        // (addr + 4)->type = 2;
        // (addr + 4)->dev_type = 3;
        // (addr + 4)->old_val.f_val = 10.5;
        // (addr + 4)->new_val.i_val = 3.2;
        // (addr + 4)->ret = 0;
        printf("Key: %d\n", addr->key);
        printf("Type: %d\n", addr->type);
        printf("Device Type: %d\n", addr->dev_type);
        printf("Old Value (float): %f\n", addr->old_val.f_val);
        printf("New Value (int): %d\n", addr->new_val.i_val);
        printf("Return Code: %d\n", addr->ret);

        char json_buffer[1024];
        // 构建要回复的JSON数据
        create_json(addr, json_buffer, num);
        printf("%s\n", json_buffer);
        // 发送HTTP响应给客户端
        send(sock, json_buffer, strlen(json_buffer), 0);

        //根据协议来判断是什么请求，然后做不同的回复处理
    
    

    
}
//结构体数组转为JSON字符串接受，发给网页
void create_json(struct std_node *addr, char *json_buffer, int *num)
{
    cJSON *root = cJSON_CreateObject();
    cJSON *dataArray = cJSON_CreateArray();

    int i = 0;
    while (i < *num)
    { // 循环处理前*num个结构体

        cJSON *dataItem = cJSON_CreateObject();
        cJSON_AddNumberToObject(dataItem, "key", addr->key);

        switch (addr->type)
        {
        case 1:
            cJSON_AddNumberToObject(dataItem, "new_val", addr->new_val.i_val);
            break;
        case 2:
            cJSON_AddNumberToObject(dataItem, "new_val", addr->new_val.b_val);
            break;
        case 3:
            cJSON_AddNumberToObject(dataItem, "new_val", addr->new_val.f_val);
            break;
        default:
            printf("Unsupported type\n");
            return;
        }

        cJSON_AddItemToArray(dataArray, dataItem);

        addr++; // 移动到下一个结构体
        i++;
    }

    cJSON_AddItemToObject(root, "data", dataArray);

    strcpy(json_buffer, cJSON_Print(root));

    cJSON_Delete(root);
}
static int handle_set(int sock, const char *input)
{
    struct std_msg msg;
    msg.msg_type = 10;
    //从input中提取整数,存储到&msg.val
    //value值
    int kk;
    msg.key = 302;
    //在input中获取%d的值放在kk中
    sscanf(input, "set=%d", &kk);
    //将 kk 转换为字符串并存储到 msg.val 中
    sprintf(msg.val, "%d", kk);

    printf("msg.key=%d msg.val=%s\n", msg.key, msg.val);

    msg_queue_send("msg", &msg, sizeof(msg), 0);
    //遍历结构体找到对应的key，修改new val值
    int ret = -1;
    ret = shm_init(&para, "shm", 1024);
    if (ret < 0)
    {
        return -1;
    }
    int *num = shm_getaddr(&para);
    //模拟
    // *num = 4;
    //
    if (num == NULL)
    {
        return -1;
    }
    struct std_node *addr = (struct std_node *)(num + sizeof(int));
    int i = 0;
    while (i < *num)
    {
        if (addr->key == 302)
        {
            if (addr->type == 1)
            {
                addr->new_val.b_val = kk;
            }
        }
        addr++; // 移动到下一个结构体
        i++;
    }
}
static int handle_set1(int sock, const char *input)
{
    struct std_msg msg;
    msg.msg_type = 10;
    //从input中提取整数,存储到&msg.val
    //value值
    int kk;
    msg.key = 301;
    //在input中获取%d的值放在kk中
    sscanf(input, "set1=%d", &kk);
    //将 kk 转换为字符串并存储到 msg.val 中
    sprintf(msg.val, "%d", kk);

    printf("msg.key=%d msg.val=%s\n", msg.key, msg.val);

    msg_queue_send("msg", &msg, sizeof(msg), 0);
    //遍历结构体找到对应的key，修改new val值
    int ret = -1;
    ret = shm_init(&para, "shm", 1024);
    if (ret < 0)
    {
        return -1;
    }
    int *num = shm_getaddr(&para);
    //模拟
    // *num = 4;
    //
    if (num == NULL)
    {
        return -1;
    }
    struct std_node *addr = (struct std_node *)(num + sizeof(int));
    int i = 0;
    while (i < *num)
    {
        if (addr->key == 301)
        {
            if (addr->type == 1)
            {
                addr->new_val.b_val = kk;
            }
        }
        addr++; // 移动到下一个结构体
        i++;
    }
}
static int handle_set2(int sock, const char *input)
{

    //寻找到和name匹配的key值
    // char *jsonCopy = "{\"version\": \"v1.0\", \"report\": { \"type\": 2, \"period\": 5 }, \"mqtt_server\": { \"addr\": \"192.168.50.92\", \"port\": 1883 }, \"mb_dev\": { \"addr\": \"192.168.31.244\", \"port\": 502 }, \"mb_app\": { \"addr\": \"192.168.31.222\", \"port\": 8887 }, \"stm32\":{ \"data\": [ { \"key\": 301, \"name\": \"js\", \"type\": 1 }, { \"key\": 302, \"name\": \"light\", \"type\": 1 }, { \"key\": 303, \"name\": \"kt\", \"type\": 1 }, { \"key\": 304, \"name\": \"temp\", \"type\": 1 }, { \"key\": 305, \"name\": \"dl\", \"type\": 2 }, { \"key\": 306, \"name\": \"temp\", \"type\": 3 } ] }, \"modbus\": { \"data\": [ { \"key\": 101, \"name\": \"hgnum\", \"addr\": 30001, \"type\": 2 }, { \"key\": 102, \"name\": \"hznum\", \"addr\": 30003, \"type\": 2 }, { \"key\": 103, \"name\": \"sdai\", \"addr\": 1, \"type\": 1 }, { \"key\": 104, \"name\": \"ywbj\", \"addr\": 40001, \"type\": 1 } ] } }";
    struct std_msg msg;
    msg.msg_type = 10;
    //从input中提取整数,存储到&msg.val
    //value值
    int kk;
    msg.key = 303;
    //在input中获取%d的值放在kk中
    sscanf(input, "set2=%d", &kk);
    //将 kk 转换为字符串并存储到 msg.val 中
    sprintf(msg.val, "%d", kk);

    printf("msg.key=%d msg.val=%s\n", msg.key, msg.val);

    msg_queue_send("msg", &msg, sizeof(msg), 0);
    //遍历结构体找到对应的key，修改new val值
    int ret = -1;
    ret = shm_init(&para, "shm", 1024);
    if (ret < 0)
    {
        return -1;
    }
    int *num = shm_getaddr(&para);
    //模拟
    // *num = 4;
    //
    if (num == NULL)
    {
        return -1;
    }
    struct std_node *addr = (struct std_node *)(num + sizeof(int));
    int i = 0;
    while (i < *num)
    {
        if (addr->key == 303)
        {
            if (addr->type == 1)
            {
                addr->new_val.b_val = kk;
            }
        }
        addr++; // 移动到下一个结构体
        i++;
    }
}
static int handle_set5(int sock, const char *input)
{

    //寻找到和name匹配的key值
    // char *jsonCopy = "{\"version\": \"v1.0\", \"report\": { \"type\": 2, \"period\": 5 }, \"mqtt_server\": { \"addr\": \"192.168.50.92\", \"port\": 1883 }, \"mb_dev\": { \"addr\": \"192.168.31.244\", \"port\": 502 }, \"mb_app\": { \"addr\": \"192.168.31.222\", \"port\": 8887 }, \"stm32\":{ \"data\": [ { \"key\": 301, \"name\": \"js\", \"type\": 1 }, { \"key\": 302, \"name\": \"light\", \"type\": 1 }, { \"key\": 303, \"name\": \"kt\", \"type\": 1 }, { \"key\": 304, \"name\": \"temp\", \"type\": 1 }, { \"key\": 305, \"name\": \"dl\", \"type\": 2 }, { \"key\": 306, \"name\": \"temp\", \"type\": 3 } ] }, \"modbus\": { \"data\": [ { \"key\": 101, \"name\": \"hgnum\", \"addr\": 30001, \"type\": 2 }, { \"key\": 102, \"name\": \"hznum\", \"addr\": 30003, \"type\": 2 }, { \"key\": 103, \"name\": \"sdai\", \"addr\": 1, \"type\": 1 }, { \"key\": 104, \"name\": \"ywbj\", \"addr\": 40001, \"type\": 1 } ] } }";
    struct std_msg msg;
    msg.msg_type = 5;
    //从input中提取整数,存储到&msg.val
    //value值
    int kk;
    msg.key = 103;
    //在input中获取%d的值放在kk中
    sscanf(input, "set5=%d", &kk);
    //将 kk 转换为字符串并存储到 msg.val 中
    sprintf(msg.val, "%d", kk);

    printf("msg.key=%d msg.val=%s\n", msg.key, msg.val);

    msg_queue_send("msg", &msg, sizeof(msg), 0);
    //遍历结构体找到对应的key，修改new val值
    int ret = -1;
    ret = shm_init(&para, "shm", 1024);
    if (ret < 0)
    {
        return -1;
    }
    int *num = shm_getaddr(&para);
    //模拟
    // *num = 4;
    //
    if (num == NULL)
    {
        return -1;
    }
    struct std_node *addr = (struct std_node *)(num + sizeof(int));
    int i = 0;
    while (i < *num)
    {
        if (addr->key == 103)
        {
            if (addr->type == 1)
            {
                addr->new_val.b_val = kk;
            }
        }
        addr++; // 移动到下一个结构体
        i++;
    }
}
static int handle_set6(int sock, const char *input)
{

    //寻找到和name匹配的key值
    // char *jsonCopy = "{\"version\": \"v1.0\", \"report\": { \"type\": 2, \"period\": 5 }, \"mqtt_server\": { \"addr\": \"192.168.50.92\", \"port\": 1883 }, \"mb_dev\": { \"addr\": \"192.168.31.244\", \"port\": 502 }, \"mb_app\": { \"addr\": \"192.168.31.222\", \"port\": 8887 }, \"stm32\":{ \"data\": [ { \"key\": 301, \"name\": \"js\", \"type\": 1 }, { \"key\": 302, \"name\": \"light\", \"type\": 1 }, { \"key\": 303, \"name\": \"kt\", \"type\": 1 }, { \"key\": 304, \"name\": \"temp\", \"type\": 1 }, { \"key\": 305, \"name\": \"dl\", \"type\": 2 }, { \"key\": 306, \"name\": \"temp\", \"type\": 3 } ] }, \"modbus\": { \"data\": [ { \"key\": 101, \"name\": \"hgnum\", \"addr\": 30001, \"type\": 2 }, { \"key\": 102, \"name\": \"hznum\", \"addr\": 30003, \"type\": 2 }, { \"key\": 103, \"name\": \"sdai\", \"addr\": 1, \"type\": 1 }, { \"key\": 104, \"name\": \"ywbj\", \"addr\": 40001, \"type\": 1 } ] } }";
    struct std_msg msg;
    msg.msg_type = 5;
    //从input中提取整数,存储到&msg.val
    //value值
    int kk;
    msg.key = 104;
    //在input中获取%d的值放在kk中
    sscanf(input, "set6=%d", &kk);
    //将 kk 转换为字符串并存储到 msg.val 中
    sprintf(msg.val, "%d", kk);

    printf("msg.key=%d msg.val=%s\n", msg.key, msg.val);

    msg_queue_send("msg", &msg, sizeof(msg), 0);
    //遍历结构体找到对应的key，修改new val值
    int ret = -1;
    ret = shm_init(&para, "shm", 1024);
    if (ret < 0)
    {
        return -1;
    }
    int *num = shm_getaddr(&para);
    //模拟
    // *num = 4;
    //
    if (num == NULL)
    {
        return -1;
    }
    struct std_node *addr = (struct std_node *)(num + sizeof(int));
    int i = 0;
    while (i < *num)
    {
        if (addr->key == 104)
        {
            if (addr->type == 1)
            {
                addr->new_val.b_val = kk;
            }
        }
        addr++; // 移动到下一个结构体
        i++;
    }
}
// static int handle_set2(int sock, const char *input)
// {
//     // 添加消息
//     struct msgbuf msg;
//     msg.type = 10;
//     sscanf(input, "set2=%dset2=%d", &msg.a, &msg.b);
//     printf("a=%d b=%d\n", msg.a, msg.b);
//     msg_queue_send("msg", &msg, sizeof(msg), 0);
// }

/**
 * @brief 处理自定义请求，在这里添加进程通信
 * @param input
 * @return
 */
int parse_and_process(int sock, const char *query_string, const char *input)
{
    //query_string不一定能用的到

    //先处理登录操作
    if (strstr(input, "username=") && strstr(input, "password="))
    {
        return handle_login(sock, input);
    }
    //处理求和请求
    else if (strstr(input, "data1=") && strstr(input, "data2="))
    {
        return handle_add(sock, input);
    }
    //处理读保持寄存器请求
    else if (strstr(input, "get"))
    {
        return handle_get(sock, input);
    }
    else if (strstr(input, "set="))
    {
        return handle_set(sock, input);
    }
    else if (strstr(input, "set1="))
    {
        return handle_set1(sock, input);
    }
    else if (strstr(input, "set2="))
    {
        return handle_set2(sock, input);
    }
    else if (strstr(input, "set5="))
    {
        return handle_set5(sock, input);
    }
    else if (strstr(input, "set6="))
    {
        return handle_set6(sock, input);
    }
    else if (strstr(input, "get2"))
    {
        return handle_get2(sock, input);
    }
    else //剩下的都是json请求，这个和协议有关了
    {
        cJSON *root = cJSON_Parse(input);
        if (cJSON_GetObjectItem(root, "echart"))
        {
            cJSON *item = cJSON_GetObjectItem(root, "middle");
            if (strcmp(item->valuestring, "all") == 0)
            {
                // printf("12345fjwiaofjafiojwafioj");
                handle_get3(sock, input);
            }
        }else if(cJSON_GetObjectItem(root, "type"))
        {
            cJSON *item = cJSON_GetObjectItem(root, "limit");
            if (strcmp(item->valuestring, "all") == 0)
            {
                // printf("12345fjwiaofjafiojwafioj");
                handle_get4(sock, input);
            }
        }
    } 

        
}

