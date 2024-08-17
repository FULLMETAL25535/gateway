/***********************************************************************************
Copy right:	    Coffee Tech.
Author:         jiaoyue
Date:           2021-11-23
Description:    提供共享内存组件
***********************************************************************************/

#include "shmem.h"

#define MAGIC_ID 'j'

/**
 * @brief 初始化共享内存
 * @param para 参数结构体，传入即可
 * @param name 共享内存标识名称
 * @return 0 -1
 */
int shm_init(struct shm_param *para, const char *name, size_t size)
{
    assert(NULL != para);
    assert(NULL != name && strlen(name) > 0);
    key_t key;
    int ret;
    int id;
    char sys_cmd[256];
    char path[256];

    sprintf(path, "/tmp/ipc/shmem/%s", name);
    sprintf(sys_cmd, "%s %s", "touch", path);
    ret = system(sys_cmd);
    UNUSED(ret);

    //创建key
    key = ftok(path, MAGIC_ID);
    if (key < 0)
    {
        perror("fail to ftok");
        printf("error :path = %s\n", path);
        return -1;
    }

    //创建共享内存
    id = shmget(key, size, IPC_CREAT | 0666);
    if (id < 0)
    {
        perror("fail to shmget");
        return -1;
    }

    para->id = id;
    para->size = size;
    strcpy(para->name, name);

    return 0;
}

/**
 * @brief 获取共享内存地址
 * @param para
 * @return 失败返回NULL
 */
void *shm_getaddr(struct shm_param *para)
{
    void *addr;
    addr = shmat(para->id, NULL, 0);
    if (addr == (void *)-1)
    {
        para->addr = NULL;
    }
    else
    {
        para->addr = addr;
    }

    return para->addr;
}

/**
 * @brief 写共享内存
 * @param para
 * @param data
 * @param size
 */
void shm_write(const struct shm_param *para, void *data, size_t size)
{
    assert(size <= para->size);
    assert(NULL != data);

    memcpy(para->addr, data, size);
}

/**
 * @brief 解除共享内存
 * @param para
 * @return
 */
int shm_del(const struct shm_param *para)
{
    assert(NULL != para);
    int ret = shmdt(para->addr);
    if (ret < 0)
    {
        perror("fail to shmdt");
        return -1;
    }

    ret = shmctl(para->id, IPC_RMID, NULL);
    if (ret < 0)
    {
        perror("fail to shmctl");
        return -1;
    }

    return 0;
}

struct shm_param shm_creat()
{
    int fd = open("./mnt/config/node.json", O_RDWR | O_CREAT | O_APPEND, 0666);
    if (fd < 0)
    {
        perror("file open err!");
    }
    char buff[2048];
    char buff1[2048];
    int n = read(fd, buff1, 2048);
    memcpy(buff, buff1, n);
    // printf("%s\n", buff);
    cJSON *datapot = cJSON_Parse(buff);
    cJSON *stm32 = cJSON_GetObjectItem(datapot, "stm32");
    cJSON *data1 = cJSON_GetObjectItem(stm32, "data");
    cJSON *modbus = cJSON_GetObjectItem(datapot, "modbus");
    cJSON *data2 = cJSON_GetObjectItem(modbus, "data");
    int num1 = cJSON_GetArraySize(data1);
    int num2 = cJSON_GetArraySize(data2);
    int num = num1 + num2;
    // printf("%d %d\n",num1,num2);
    struct shm_param msg;
    if (shm_init(&msg, "shereme", sizeof(int) + sizeof(struct std_node) * num) < 0)
    {
        printf("shm init err\n");
    }
    int *nodenum = (int *)shm_getaddr(&msg);
    struct std_node *stm = (struct std_node *)(nodenum + 1);
    struct std_node *mod = (struct std_node *)(nodenum + 1) + num1;

    for (int i = 0; i < num1; i++)
    {
        cJSON *item = cJSON_GetArrayItem(data1, i);
        cJSON *key1 = cJSON_GetObjectItem(item, "key");
        (stm + i)->key = key1->valueint;
        key1 = cJSON_GetObjectItem(item, "type");
        (stm + i)->type = key1->valueint;
        (stm + i)->dev_type = 1;
    }
    for (int i = 0; i < num2; i++)
    {
        cJSON *item = cJSON_GetArrayItem(data2, i);
        cJSON *key1 = cJSON_GetObjectItem(item, "key");
        (mod + i)->key = key1->valueint;
        key1 = cJSON_GetObjectItem(item, "type");
        (mod + i)->type = key1->valueint;
        (mod + i)->dev_type = 2;
    }
    return msg;
}