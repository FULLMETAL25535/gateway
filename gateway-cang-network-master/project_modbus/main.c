#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include "mb.h"
#include "cJSON.h"
#include "list.h"
#include <sys/stat.h>
// 声明链表和互斥锁
struct list_head mb_list;
pthread_mutex_t node_list_mutex = PTHREAD_MUTEX_INITIALIZER; // 互斥锁，用于保护链表的访问

// 从文件中读取 JSON 内容
char *read_file(const char *filename) 
{
    FILE *file = fopen(filename, "r");
    if (file == NULL) 
    {
        printf("Cannot open file: %s\n", filename);
        return NULL;
    }
    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);
    char *content = (char *)malloc(length + 1);
    if (content == NULL) 
    {
        printf("Memory allocation failed\n");
        fclose(file);
        return NULL;
    }
    fread(content, 1, length, file);
    content[length] = '\0';
    fclose(file);
    return content;
}

// 数据采集线程
void *data_collection_thread(void *arg)
{
    printf("数据采集线程启动\n");
    modbus_t *ctx;

    // 创建Modbus TCP上下文
    ctx = modbus_new_tcp(SERVER_IP, SERVER_PORT);
    if (ctx == NULL) 
    {
        fprintf(stderr, "Unable to allocate libmodbus context\n");
        pthread_exit(NULL);
    }

    // 设置从机ID
    modbus_set_slave(ctx, SERVER_ID);

    // 建立连接
    if (modbus_connect(ctx) == -1) 
    {
        fprintf(stderr, "Modbus Connection failed\n");
        modbus_free(ctx);
        pthread_exit(NULL);
    }

    printf("数据采集线程连接成功\n");

    // 初始化共享内存
    struct std_node sendtoshm;
    struct std_node *psendtoshm;   
    
    if (shm_init(&sendtoshm, SHM_NAME, SHM_SIZE) < 0)
    {
        perror("shm init err");
        return NULL;
    }

    // 获取共享内存地址
    int *addr_shm = shm_getaddr(&sendtoshm);
    if (addr_shm == NULL)
    {
        perror("shm getaddr err");
        return NULL;
    }    
    // 循环采集数据
    while (1) 
    {
        psendtoshm = (struct std_node *)(addr_shm+1);
        pthread_mutex_lock(&node_list_mutex); // 锁定互斥锁
        struct list_head *pos;

        printf("数据采集线程采集到的数据\n");
        for(int i = 0;i < *addr_shm;i++)
        {
            list_for_each(pos, &mb_list)
                {
                    struct mb_node_list *entry = list_entry(pos, struct mb_node_list, list);

                    if (psendtoshm->key == entry->node.key) 
                    {
                        if(psendtoshm->type == 2)
                        {
                            modbus_read_registers(ctx, entry->node.addr, 1, (uint16_t *)&psendtoshm->new_val.i_val);
                            printf("读到了key值为%d的寄存器设备\n",psendtoshm->key);
                        }                        
                        else if(psendtoshm->type == 1)
                        {
                            modbus_read_bits(ctx, entry->node.addr, 1, (uint8_t *)&psendtoshm->new_val.b_val);
                            printf("读到了key值为%d的线圈设备\n",psendtoshm->key);
                        }                        
                    }         
                }
            psendtoshm = psendtoshm + 1;
        }


        printf("数据采集线程采集完毕\n");
        printf("-----------------\n");
        pthread_mutex_unlock(&node_list_mutex); // 释放互斥锁
        sleep(1);
    }

    modbus_close(ctx);
    modbus_free(ctx);
    pthread_exit(NULL);

}

// 指令处理线程函数,从消息队列中获取指令
void *instruction_processing_thread(void *arg) 
{
    printf("指令处理线程启动\n");
    modbus_t *ctx;
    ctx = modbus_new_tcp(SERVER_IP, SERVER_PORT);
    if (ctx == NULL) 
    {
        fprintf(stderr, "Unable to allocate libmodbus context\n");
        pthread_exit(NULL);
    }

    // 设置从机ID
    modbus_set_slave(ctx, SERVER_ID);

    // 建立连接
    if (modbus_connect(ctx) == -1) 
    {
        fprintf(stderr, "Modbus Connection failed\n");
        modbus_free(ctx);
        pthread_exit(NULL);
    }

    printf("指令处理线程连接成功\n");

    // 不断处理控制指令
    while (1) 
    {
        // 接收控制指令
        struct std_msg savemsg;

        if (msg_queue_recv(MSG_QUEUE_NAME, &savemsg, sizeof(savemsg), 5, 0) == -1) 
        {
            perror("接收消息失败");
            //exit(EXIT_FAILURE);
        }

        printf("接收到消息需要修改的值为: %s-----需要修改的设备的key值为%d\n", savemsg.val,savemsg.key);
        
        pthread_mutex_lock(&node_list_mutex); // 加锁，保护链表访问
        // 查找key对应的寄存器地址
        struct list_head *pos;
        list_for_each(pos, &mb_list) 
        {
            struct mb_node_list *entry = list_entry(pos, struct mb_node_list, list);
            if (entry->node.key == savemsg.key) 
            {
                if (modbus_write_bit(ctx, entry->node.addr, atoi(savemsg.val)) == -1) 
                {
                    fprintf(stderr, "Failed to write bit for key %d\n", savemsg.key);
                } 
                else 
                {
                    printf("Key %d 修改成功\n", savemsg.key);
                }
                break; // 找到对应key后退出循环
            }
        }

        pthread_mutex_unlock(&node_list_mutex); // 解锁

        sleep(1);
    }

    // 关闭Modbus连接并释放资源
    modbus_close(ctx);
    modbus_free(ctx);
    pthread_exit(NULL);
}


int main() 
{
    // 初始化链表头
    //struct list_head mb_list;
    INIT_LIST_HEAD(&mb_list);

    // 读取 JSON 文件内容
    while (1)
    {
        struct stat st;
        if (stat("/mnt/config/node.json", &st) < 0)
        {
            perror("stat err");
            continue;
        }
        if (st.st_size < 100){
            sleep(1);
            printf("modubs等待点表\n");
            continue;
        } 
        else 
            break;
    }
    char *json_str = read_file("/mnt/config/node.json");
    if (json_str == NULL) 
    {
        return -1;
    }

    // 解析并插入 modbus 数据到链表
    parse_and_insert_modbus_data(json_str, &mb_list);

    // 释放读取的 JSON 字符串内存
    free(json_str);

    // 遍历并打印链表中的数据
    struct list_head *pos;
    list_for_each(pos, &mb_list)
    {
        struct mb_node_list *entry = list_entry(pos, struct mb_node_list, list);
        printf("Key: %d, Name: %s, Type: %d, Addr: %d\n", entry->node.key, entry->node.name, entry->node.type, entry->node.addr);
    }

    // 创建数据采集线程和指令处理线程
    pthread_t data_thread, instruction_thread;
    pthread_create(&data_thread, NULL, data_collection_thread, NULL);
    pthread_create(&instruction_thread, NULL, instruction_processing_thread, NULL);

    // 主线程等待子线程结束
    pthread_join(data_thread, NULL);
    pthread_join(instruction_thread, NULL);

    // 释放链表中的所有节点
    struct list_head *n;
    list_for_each_safe(pos, n, &mb_list) 
    {
        struct mb_node_list *entry = list_entry(pos, struct mb_node_list, list);
        list_del(pos);
        free(entry);
    }

    return 0;
}
