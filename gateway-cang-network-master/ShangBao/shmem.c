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
    if(key < 0){
        perror("fail to ftok");
        printf("error :path = %s\n", path);
        return -1;
    }

    //创建共享内存
    id = shmget(key, size, IPC_CREAT|0666);
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
    if(addr == (void *)-1)
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
    if(ret < 0)
    {
        perror("fail to shmdt");
        return -1;
    }

    ret = shmctl(para->id, IPC_RMID, NULL);
    if(ret < 0)
    {
        perror("fail to shmctl");
        return -1;
    }

    return 0;
}
