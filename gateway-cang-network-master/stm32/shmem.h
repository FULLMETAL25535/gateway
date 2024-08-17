/***********************************************************************************
Copy right:	    Coffee Tech.
Author:         jiaoyue
Date:           2021-11-23
Description:    提供共享内存组件
***********************************************************************************/

#ifndef SHMEM_H
#define SHMEM_H

#include "pub_define.h"
#include <sys/ipc.h>
#include <sys/shm.h>

#define SHM_NAME_SZ    32

struct shm_param
{
    int id;                       //共享内存ID
    size_t size;
    void *addr;                   //共享内存地址
    char name[SHM_NAME_SZ+1];     //共享内存key标识
};

int shm_init(struct shm_param *para, const char *name, size_t size);
void *shm_getaddr(struct shm_param *para);
void shm_write(const struct shm_param *para, void *data, size_t size);
int shm_del(const struct shm_param *para);

#endif  // SHMEM_H
