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

#define N 64
// void cjson_handle(cJSON *cjson);
void put_server(int clifd, char *buf, int size);
int main(int argc, char const *argv[])
{
    int broadfd;

    //创建一个socket文件描述符
    broadfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (broadfd < 0)
    {
        perror("sock err");
        return -1;
    }

    //绑定套接字(ip+port)
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(5678); //端口号
    addr.sin_addr.s_addr = INADDR_ANY;

    int addrlen = sizeof(addr);

    if (bind(broadfd, (struct sockaddr *)&addr, addrlen) < 0)
    {
        perror("bind err");
        return -1;
    }

    ssize_t len;
    char buf[N] = {0};
    struct sockaddr_in cliaddr;

    //接收广播包
    printf("准备接收\n");
    bzero(buf, N);
    len = recvfrom(broadfd, buf, N, 0, (struct sockaddr *)&cliaddr, &addrlen);
    printf("接收成功\n");

    //判断是否是本公司产品：one piece
    if (strcmp(buf, "yhy") != 0)
    {
        printf("not my\n");
    }
    printf("%s\n", buf);

    //回复yes，告诉软件，我收到了搜索协议，并且回复地址
    sendto(broadfd, "yes", 4, 0, (struct sockaddr *)&cliaddr, addrlen);

    //变身为TCP服务器，准备接收 点表 心跳
    int tcpfd = socket(AF_INET, SOCK_STREAM, 0);
    if (tcpfd < 0)
    {
        perror("sock err");
        return -1;
    }

    if (bind(tcpfd, (struct sockaddr *)&addr, addrlen) < 0)
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

    //接收客户端的连接
    printf("wait client connect\n");
    int clifd;
    clifd = accept(tcpfd, NULL, NULL);
    if (clifd < 0)
    {
        perror("accept err");
        return -1;
    }
    printf("new cononect coming\n");
    char json[2048] = {0};
    while (1)
    {
        //接收对端的地址

        //接收json结构信息
        if (recv(clifd, buf, N, 0) < 0)
        {
            perror("recv err");
            return -1;
        }
        cJSON *cjson = cJSON_Parse(buf);
        if (NULL == cjson)
        {
            printf("not json\n");
            return -1;
        }
        char *root = cJSON_Print(cjson);
        printf("%s\n", root);
        for (int i = 0; i < cJSON_GetArraySize(cjson); i++) //遍历最外层json键值对
        {
            cJSON *item = cJSON_GetArrayItem(cjson, i);
            int type;

            if (strcmp(item->string, "type") == 0)
            {
                type = atoi(cJSON_Print(item));
                if (type == 1)
                {
                    put_server(clifd, json, 2048);
                    printf("%s\n", json);
                    memset(json, 0, 2048);
                }
                // if (type == 2)
                // {

                // }
                printf("判断 %d\n", type);
                if (type == 3)
                {
                    //创建一个空对象
                    cJSON *hjson = cJSON_CreateObject();
                    //添加字符键值对
                    cJSON_AddNumberToObject(hjson,"type",3);
                    cJSON_AddNumberToObject(hjson,"result",0);
                    char *p = cJSON_Print(hjson);
                    printf("%s\n", p);
                    send(clifd, p, strlen(p), 0);
                }
            }
            printf("has handel\n");
        }
    }
    close(clifd);
    close(tcpfd);
    return 0;
}
void put_server(int clifd, char *buf, int size)
{
    int fd = open("./node.json", O_RDONLY);
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
            return ;
        }
        char *root = cJSON_Print(cjson);
        printf("%s", root);
        send(clifd, root, strlen(root), 0);
    }
    printf("发送完毕\n");
}
// void cjson_handle(cJSON * cjson)//以递归的方式打印json的最内层键值对
// {
// 	int type;
// 	for(int i=0; i<cJSON_GetArraySize(cjson); i++)   //遍历最外层json键值对
// 	{
// 		cJSON * item = cJSON_GetArrayItem(cjson, i);
// 		if(item->type == cJSON_Object)//如果对应键的值仍为cJSON_Object就递归调用printJson
// 		{
// 			printJson(item);
// 		}else
// 			if(item->type == cJSON_Array)//如果对应键的值仍为cJSON_Object就递归调用printJson
// 			{
// 				printJson(item);
// 			}
// 			else  //值不为json对象就直接打印出键和值
// 			{
// 				if(strcmp(item->string, "key") == 0)
// 				{
// 					node = (struct data *)malloc(sizeof(struct data));
// 					node->key = atoi(cJSON_Print(item));
// 				}
// 				if(strcmp(item->string, "type") == 0)
// 				{
// 					node->type = atoi(cJSON_Print(item));
// 					type = atoi(cJSON_Print(item));
// 				}
// 				if(strcmp(item->string, "val") == 0)
// 				{
// 					if(type == 1)
// 					{
// 						node->val.b_val = atoi(item->valuestring);
// 						list_add(&node->list, &head);
// 					}
// 					if(type == 2)
// 					{
// 						node->val.i_val = atoi(item->valuestring);
// 						list_add(&node->list, &head);
// 					}
// 					if(type == 3)
// 					{
// 						node->val.f_val = atof(item->valuestring);
// 						list_add(&node->list, &head);
// 					}

// 				}
// 				printf("%s->", item->string);
// 				printf("%s\n", cJSON_Print(item));
// 				//	printf("key = %d, type = %d\n", node->key, node->type);
// 			}
// 	}
// }