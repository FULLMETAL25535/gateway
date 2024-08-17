/***********************************************************************************
Copy right:	    Coffee Tech.
Author:         jiaoyue
Date:           2019-11-21
Description:    点对点型消息队列组件
***********************************************************************************/

#include <errno.h>
#include "msg_queue_peer.h"

#define MSG_PATH "/tmp/ipc/msgqueue/peer/"

#define MAGIC_ID 'j'

/**
 * @brief msg_queue_send
 * @param name 发送给哪个消息队列
 * @param msg 消息数据，第一个字段必须是long类型的消息类型
 * @param msgsz 整个msg大小，不需要减去long--sizeof(*msg)
 * @param msgflg 同msgsnd
 * @return 同msgsnd
 */
int msg_queue_send(const char *name, const void *msg, size_t msgsz, int msgflg)
{
    assert(NULL != name && strlen(name) > 0);
    assert(NULL != msg);

    key_t key;
    int ret;
    int msgid;
    char sys_cmd[256];
    char path[256];

    sprintf(path, "%s%s", MSG_PATH, name);

    //文件不存在创建
    if(access(path, F_OK) < 0)
    {
        sprintf(sys_cmd, "%s %s", "touch", path);
        ret = system(sys_cmd);
        UNUSED(ret);
    }

    //创建key
    key = ftok(path, MAGIC_ID);
    if(key < 0){
        perror("fail to ftok");
        return -1;
    }

    //创建消息队列
    msgid = msgget(key, IPC_CREAT|0666);
    if (msgid < 0)
    {
        perror("fail to msgget");
        return -1;
    }

    return msgsnd(msgid, msg, msgsz - sizeof(long), msgflg);
}

/**
 * @brief msg_queue_send
 * @param name 从哪个消息队列接收
 * @param msg 消息缓冲区，第一个字段必须是long类型的消息类型
 * @param msgsz 整个msg大小，不需要减去long--sizeof(*msg)
 * @param msgtyp 同msgrcv
 * @param msgflg 同msgrcv
 * @return 同msgrcv
 */
int msg_queue_recv(const char *name, void *msg, size_t msgsz, long msgtyp, int msgflg)
{
    assert(NULL != name && strlen(name) > 0);
    assert(NULL != msg);

    key_t key;
    int ret;
    int msgid;
    char sys_cmd[256];
    char path[256];

    sprintf(path, "%s%s", MSG_PATH, name);
    //文件不存在创建
    if(access(path, F_OK) < 0)
    {
        sprintf(sys_cmd, "%s %s", "touch", path);
        ret = system(sys_cmd);
        UNUSED(ret);
    }

    //创建key
    key = ftok(path, MAGIC_ID);
    if(key < 0){
        perror("fail to ftok");
        return -1;
    }

    //创建消息队列
    msgid = msgget(key, IPC_CREAT|0666);
    if (msgid < 0)
    {
        perror("fail to msgget");
        return -1;
    }

    return msgrcv(msgid, msg, msgsz - sizeof(long), msgtyp, msgflg);
}


/**
 * @brief 判断某个类型的消息是否存在，但是不取出
 * @param name 指定消息队列名
 * @param msgtyp 消息类型
 * @return TRUE FALSE
 */
BOOL msg_queue_msgexist(const char *name, long msgtyp)
{
    assert(NULL != name && strlen(name) > 0);

    key_t key;
    int ret;
    int msgid;
    char sys_cmd[256];
    char path[256];

    sprintf(path, "%s%s", MSG_PATH, name);
    //文件不存在创建
    if(access(path, F_OK) < 0)
    {
        sprintf(sys_cmd, "%s %s", "touch", path);
        ret = system(sys_cmd);
        UNUSED(ret);
    }

    //创建key
    key = ftok(path, MAGIC_ID);
    if(key < 0)
    {
        perror("fail to ftok");
        return -1;
    }

    //创建消息队列
    msgid = msgget(key, IPC_CREAT|0666);
    if (msgid < 0)
    {
        perror("fail to msgget");
        return -1;
    }

    if(msgrcv(msgid, NULL, 0, msgtyp, IPC_NOWAIT) < 0)
    {
        if(errno == E2BIG)
        {
            return TRUE;
        }
    }
    return FALSE;
}

