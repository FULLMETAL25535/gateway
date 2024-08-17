#include <stdio.h>
#include <stdlib.h>
#include "mb.h"
#include "cJSON.h"
#include "list.h"

void send_message(char *msg_text) 
{
    struct std_msg test;
    test.msg_type = 5;  // 消息类型设为1
    test.key = 103;
    //strncpy(test.val, msg_text, MESSAGE_SIZE);
    snprintf(test.val, MESSAGE_SIZE, "%s", msg_text);

    if (msg_queue_send(MSG_QUEUE_NAME, &test, sizeof(test), 0) == -1) {
        perror("发送消息失败");
        exit(EXIT_FAILURE);
    }

    printf("消息发送成功: %s\n", msg_text);
}

int main()
{
    while(1)
    {
        send_message("1");
        send_message("0");
        sleep(5);
    }
    return 0;
}