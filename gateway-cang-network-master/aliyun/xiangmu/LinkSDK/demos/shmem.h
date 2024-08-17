/***********************************************************************************
Copy right:	    Coffee Tech.
Author:         jiaoyue
Date:           2021-11-23
Description:    提供共享内存组件
***********************************************************************************/

#ifndef SHMEM_H
#define SHMEM_H

#include "data.h"

#define SHM_NAME_SZ    32

struct shm_param
{
    int id;                       //共享内存ID
    size_t size;
    void *addr;                   //共享内存地址
    char name[SHM_NAME_SZ+1];     //共享内存key标识
};

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

int shm_init(struct shm_param *para, const char *name, size_t size);
void *shm_getaddr(struct shm_param *para);
void shm_write(const struct shm_param *para, void *data, size_t size);
int shm_del(const struct shm_param *para);

struct shm_param shm_creat();

#endif  // SHMEM_H
