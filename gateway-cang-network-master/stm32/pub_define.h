/***********************************************************************************
Copy right:	    Coffee Tech.
Author:         jiaoyue
Date:           2019.7.30
Description:    类型重定义
***********************************************************************************/
#ifndef __PUB_DEFINE_H
#define __PUB_DEFINE_H

#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>

#define MAX_PROC_NAME_SZ    32

#define MAX_PATH_LEN 256  //最大路径长度

#ifdef BOOL
#undef BOOL
#endif

#ifdef FALSE
#undef FALSE
#endif

#ifdef TRUE
#undef TRUE
#endif

#define BOOL    int
#define FALSE   (0)
#define TRUE    (!FALSE)

#define KB 1024
#define MB (1024*KB)
#define GB (1024*MB)

#define UNUSED(x) (void)x;  //仅仅为了消除警告用，可解决未定义变量

#include <stdint.h>

#define uint8  uint8_t
#define uint16 uint16_t
#define uint32 uint32_t

#define int8  int8_t
#define int16 int16_t
#define int32 int32_t

#define SLEEP_MS(ms) usleep(ms*1000)

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(*(x)))

#endif //__PUB_DEFINE_H
