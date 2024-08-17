/***********************************************************************************
Copy right:	    Coffee Tech.
Author:         jiaoyue
Date:           2022-03-24
Description:    http请求自定义处理部分
***********************************************************************************/

#ifndef CUSTOM_HANDLE_H
#define CUSTOM_HANDLE_H

#include <stdio.h>
#include <string.h>
#include "cJSON.h"
#include <unistd.h>
#include <sys/utsname.h>
#include <sqlite3.h>
#include <errno.h>
#include <stdlib.h>
// 数据库连接参数

int parse_and_process(int sock, const char *query_string, const char *input);

#endif  // CUSTOM_HANDLE_H
