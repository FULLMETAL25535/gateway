/***********************************************************************************
Copy right:	    Coffee Tech.
Author:         jiaoyue
Date:           2019-11-21
Description:    点对点型消息队列组件
***********************************************************************************/

#ifndef MSG_QUEUE_PEER_H
#define MSG_QUEUE_PEER_H

#include "pub_define.h"
#include <sys/ipc.h>
#include <sys/msg.h>

int msg_queue_send(const char *name, const void *msg, size_t msgsz, int msgflg);
int msg_queue_recv(const char *name, void *msg, size_t msgsz, long msgtyp, int msgflg);

#endif  // MSG_QUEUE_PEER_H
