#include <stdio.h>
#include <sys/types.h> /* See NOTES */
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h> /* superset of previous */
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include "cJSON.h"
#include <pthread.h>
#include <sys/epoll.h>

#define N 1024

// void cjson_handle(cJSON *cjson);
void put_server(int clifd, char *buf, int size);

void *new_sock(void *arg)
{
    printf("will recv new connect\n");
    int sockfd, clientfd;
    char buf[N];
    int addrlen = sizeof(struct sockaddr);
    struct sockaddr_in addr, clientaddr;
    int nbytes;
    // 1创建一个套接字--socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        perror("socket err");
        exit(-1);
    }

    // 2定义套接字地址--sockaddr_in
    bzero(&addr, addrlen);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(5678);  //新的连接用6666端口

    // 3绑定套接字--bind
    if (bind(sockfd, (struct sockaddr *)&addr, addrlen) < 0)
    {
        perror("bind err");
        exit(-1);
    }
    // 4启动监听--listen
    if (listen(sockfd, 5) < 0)
    {
        perror("listen err");
        exit(-1);
    }

    // 5接收连接--accept
    clientfd = accept(sockfd, (struct sockaddr *)&clientaddr, &addrlen);
    if (clientfd < 0)
    {
        perror("accept err");
        exit(-1);
    }

    printf("recv new client\n");
    char filename[32] = {0};

    recv(clientfd, buf, N, 0);

    //接收文件名
    printf("recv %s-%d data = %s\n", inet_ntoa(clientaddr.sin_addr),
           ntohs(clientaddr.sin_port), buf);
    cJSON *root = cJSON_Parse(buf);
    cJSON *data = cJSON_GetObjectItem(root, "data");
    cJSON *item = cJSON_GetObjectItem(data, "file_name");
    strcpy(filename, item->valuestring);
    item = cJSON_GetObjectItem(data, "file_len");
    int len1 = item->valueint;
    //回复start
    root = cJSON_CreateObject();
    cJSON_AddItemToObject(root, "type", cJSON_CreateNumber(2));
    data = cJSON_CreateObject();
    cJSON_AddItemToObject(root, "data", data);
    cJSON_AddItemToObject(data, "flag", cJSON_CreateString("start"));
    char *p = cJSON_Print(root);
    send(clientfd, p, strlen(p), 0);

    // printf("p=%s\n",p);
    //接收文件

    int fd = open("/mnt/config/node.json", O_RDWR | O_CREAT, 0777);
    if (fd < 0)
    {
        perror("open err");
    }
    while (1)
    {
        printf("111\n");
        bzero(buf, N);
        int len = recv(clientfd, buf, N, 0);
        if (len < 0)
        {
            perror("recv err");
            break;
        }
        else if (len == 0)
        {
            printf("file send complete\n");
            break;
        }
        else
        {
            write(fd, buf, len);
        }
    }

    //回复stop
    root = cJSON_CreateObject();
    cJSON_AddItemToObject(root, "type", cJSON_CreateNumber(2));
    data = cJSON_CreateObject();
    cJSON_AddItemToObject(root, "data", data);
    cJSON_AddItemToObject(data, "flag", cJSON_CreateString("stop"));
    p = cJSON_Print(root);
    send(clientfd, p, strlen(p), 0);

    // printf("p=%s\n",p);
}

void *mpthread(void *arg)
{
    //接收json结构信息
    int tcpfd = *((int *)arg);
    char buf[N];
    char json[2048];
    ssize_t len;
    int clifd;
    cJSON *cjson;
    cJSON *item ;
    struct sockaddr_in cliaddr;
    int addrlen = sizeof(cliaddr);

    //接收对端的地址
    printf("wait client connect\n");

    clifd = accept(tcpfd, (struct sockaddr *)&cliaddr, &addrlen);
    if (clifd < 0)
    {
        perror("accept err");
        return NULL;
    }
    printf("new cononect coming\n");
    printf("tcpfd:%d  clifd:%d\n", tcpfd, clifd);
    printf("client:ip=%s port=%d\n", inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port));
    while (1)
    {
        int rec = recv(clifd, buf, N, 0); //定义一个值接收 只能读一遍
        if (rec < 0)
        {
            perror("recv err");
            return NULL;
        }
        else if (rec == 0)
        {
            printf("client exit:%d\n", clifd);
            break;
        }

        cjson = cJSON_Parse(buf);
        if (NULL == cjson)
        {
            printf("not json\n");
            continue;
        }
        char *root = cJSON_PrintUnformatted(cjson);
        printf("%s\n", root);
        for (int i = 0; i < cJSON_GetArraySize(cjson); i++) //遍历最外层json键值对
        {
            item = cJSON_GetArrayItem(cjson, i);
            int type;

            if (strcmp(item->string, "type") == 0)
            {
                type = atoi(cJSON_PrintUnformatted(item));
                printf("判断 %d\n", type);
                if (type == 1)
                {
                    put_server(clifd, json, 2048);//发送点表的函数
                    // printf("%s\n", json);
                    memset(json, 0, 2048);
                }
                if (type == 2)
                {
                    // printf("222\n");
                    pthread_t tid;
                    pthread_create(&tid, NULL, new_sock, NULL);
                }

                if (type == 3)
                {
                    //创建一个空对象
                    cJSON *hjson = cJSON_CreateObject();
                    //添加字符键值对
                    cJSON_AddNumberToObject(hjson, "type", 3);
                    cJSON_AddNumberToObject(hjson, "result", 0);
                    char *p = cJSON_PrintUnformatted(hjson);
                    printf("%s\n", p);
                    send(clifd, p, strlen(p), 0);
                    memset(buf, 0, N);
                    free(p);
                    cJSON_Delete(hjson);
                }
            }
        }
        free(root);
    }
    printf("clifd has handel:%d\n", clifd);
    
    close(clifd);
    cJSON_Delete(cjson);
    pthread_exit(NULL);
}

int main(int argc, char const *argv[])
{
    //创建一个socket文件描述符
    int broadfd;
    broadfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (broadfd < 0)
    {
        perror("sock err");
        return -1;
    }

    //绑定套接字(ip+port)
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8886); //端口号
    addr.sin_addr.s_addr = INADDR_ANY;
    struct sockaddr_in cliaddr;
    struct sockaddr_in tcpaddr = addr;
    int addrlen = sizeof(addr);

    //作为服务器绑定
    if (bind(broadfd, (struct sockaddr *)&addr, addrlen) < 0)
    {
        perror("bind err");
        return -1;
    }

    //变身为TCP服务器，准备接收 点表 心跳
    int tcpfd = socket(AF_INET, SOCK_STREAM, 0);
    if (tcpfd < 0)
    {
        perror("sock err");
        return -1;
    }

    if (bind(tcpfd, (struct sockaddr *)&tcpaddr, addrlen) < 0)
    {
        perror("bind err");
        return -1;
    }

    //监听套接字
    if (listen(tcpfd, 5) < 0)
    {
        perror("listen err");
        return -1;
    }
    
    ssize_t len;
    char *qt_addr;
    int qt_port;
    char buf[N];
    
    while (1)
    {
        //接收广播包
        printf("准备接收\n");
        bzero(buf, N);
        len = recvfrom(broadfd, buf, N, 0, (struct sockaddr *)&cliaddr, &addrlen);
        if (qt_port == ntohs(cliaddr.sin_port) && strcmp(qt_addr, inet_ntoa(cliaddr.sin_addr)) == 0)
        {
            printf("相同客户\n");
            continue;
        }
        else
        {
            printf("接收成功\n");
            qt_addr = inet_ntoa(cliaddr.sin_addr);
            qt_port = ntohs(cliaddr.sin_port);

            //判断是否是本公司产品：one piece
            if (strncmp(buf, "yhy", 3) != 0)
            {
                printf("not my\n");
            }
            printf("%s\n", buf);

            //回复yes，告诉软件，我收到了搜索协议，并且回复地址
            sendto(broadfd, "yes", 4, 0, (struct sockaddr *)&cliaddr, addrlen);

            //多线程
            pthread_t tid;
            pthread_create(&tid, NULL, mpthread, &tcpfd);
            pthread_detach(tid);
        }
    }
    close(broadfd);
    close(tcpfd);
    return 0;
}

void put_server(int clifd, char *buf, int size)
{
    int fd = open("/mnt/config/node.json", O_RDONLY);
    if (fd < 0)
    {
        perror("open err");
        return;
    }

    int ret;
    while (ret = read(fd, buf, size - 1))
    {
        buf[ret] = '\0';
        cJSON *cjson = cJSON_Parse(buf);
        if (NULL == cjson)
        {
            printf("not json\n");
            return;
        }
        char *root = cJSON_PrintUnformatted(cjson);
        // printf("%s", root);
        send(clifd, root, strlen(root), 0);
        
        free(root);
        cJSON_Delete(cjson);
    }
    printf("发送完毕\n");
}
