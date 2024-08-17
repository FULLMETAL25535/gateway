#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>  /* See NOTES */
#include <netinet/ip.h> /* superset of previous */
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/select.h>
#include <sys/time.h>
#include <pthread.h>
#include "cJSON.h"
#include "shmem.h"
#include "msg_queue_peer.h"

#define N 4096
char buf[N] = {0};  // 存放客户端发来的消息
char buf2[N] = {0}; // 存放点表读出的数据
// 消息队列
struct std_msg
{
    long msg_type; //消息类型
    int key;       //唯一键值
    char val[32];  //需要修改成的数据
};
union val_t {
    int b_val;   //bool类型存储空间
    int i_val;   //整形值存储空间
    float f_val; //浮点值存储空间
};
struct std_node
{
    int key;
    int type;
    int dev_type;
    union val_t old_val;
    union val_t new_val;
    int ret;
};
struct std_node *st;
int clifd; // 客户端
// 线程接收代码
void *handler_thread_read()
{
    // 上位机发信息一次只能发一条控制信息
    // 给stm32发的消息要把消息类型设置为专门只有你能收的消息类型
    // 你就读这个消息类型（要先定义一个结构体读，你们两边的结构体要一样）
    // 然后看看他要控制哪个灯，再看要开还是要关，自己做个序列化发给单片机
    // 单片机再读取出来反序列化，判断是干什么
    while (1)
    {   printf("123\n");
        struct std_msg m;
        msg_queue_recv("msg", &m, sizeof(struct std_msg), 10, 0);
        printf("123456\n");
        cJSON *root = cJSON_CreateObject();
        cJSON_AddItemToObject(root, "key", cJSON_CreateNumber(m.key));
        cJSON_AddItemToObject(root, "val", cJSON_CreateNumber(m.val[0] - 48));
        printf("%s\n", m.val);
        char *p = cJSON_Print(root);
        printf("%s\n",p);
        send(clifd, p, 128, 0);
        cJSON_Delete(root);
    }
}
int main(int argc, char const *argv[])
{
    while (1)
    {
        struct stat st;
        if (stat("/mnt/config/node.json", &st) < 0)
        {
            perror("stat err");
            continue;
        }
        if (st.st_size < 100){
            sleep(1);
            printf("stm32等待点表\n");
            continue;
        } 
        else 
            break;
    }
    // 反序列化点表，获取相应的key值
    FILE *fp = fopen("/mnt/config/node.json", "r");
    if (NULL == fp)
    {
        perror("fopen err"); //如果以r方式打开，没有这个文件会报错，打印错误信息。
        return -1;
    }
    fread(buf2, 1, N, fp);
    cJSON *root = cJSON_Parse(buf2);
    cJSON *stm = cJSON_GetObjectItem(root, "stm32");
    cJSON *data = cJSON_GetObjectItem(stm, "data");
    // 获得加湿器key值
    cJSON *d = cJSON_GetArrayItem(data, 0);
    cJSON *t = cJSON_GetObjectItem(d, "key");
    int j1 = t->valueint;
    // 获得灯key值
    d = cJSON_GetArrayItem(data, 1);
    t = cJSON_GetObjectItem(d, "key");
    int l1 = t->valueint;
    // 获得空调key值
    d = cJSON_GetArrayItem(data, 2);
    t = cJSON_GetObjectItem(d, "key");
    int k1 = t->valueint;
    // 获得湿度key值
    d = cJSON_GetArrayItem(data, 3);
    t = cJSON_GetObjectItem(d, "key");
    int t1 = t->valueint;
    // 获得电量key值
    d = cJSON_GetArrayItem(data, 4);
    t = cJSON_GetObjectItem(d, "key");
    int d1 = t->valueint;
    // 获得湿度key值
    d = cJSON_GetArrayItem(data, 5);
    t = cJSON_GetObjectItem(d, "key");
    int t2 = t->valueint;

    // 创建套接字
    int serverfd = socket(AF_INET, SOCK_STREAM, 0);
    if (serverfd < 0)
    {
        perror("socket err");
        return -1;
    }

    // 绑定自己的地址
    struct sockaddr_in myaddr;
    socklen_t addrlen = sizeof(myaddr);
    memset(&myaddr, 0, addrlen);
    myaddr.sin_family = AF_INET;
    myaddr.sin_port = htons(8888);
    myaddr.sin_addr.s_addr = INADDR_ANY;

    int ret = bind(serverfd, (struct sockaddr *)&myaddr, addrlen);
    if (ret < 0)
    {
        perror("bind err");
        return -1;
    }
    // 启动监听
    ret = listen(serverfd, 5);
    if (ret < 0)
    {
        perror("bind err");
        return -1;
    }
    // 创建线程 主线程接收客户端的信息，从线程接收命令
    pthread_t tid;
    if (pthread_create(&tid, NULL, handler_thread_read, NULL) != 0)
    {
        perror("create thread err");
        return -1;
    }
    //构造文件描述符表
    fd_set rdfds, tmpfds;
    FD_ZERO(&rdfds); //清表
    FD_SET(serverfd, &rdfds);

    int maxfd = serverfd;
    while (1)
    {
        //后续客户端连入或者退出，只让rdfds变化，然后select每次改变的是临时的一张表：tmpfds
        tmpfds = rdfds;
        select(maxfd + 1, &tmpfds, NULL, NULL, NULL);
        //代码运行到这，代表表里某个设备有数据：服务器fd，客户端fd
        for (size_t i = 0; i < maxfd + 1; i++)
        {
            //如果这个成立，代表当前表这个描述符有数据
            if (FD_ISSET(i, &tmpfds))
            {
                //判断这个描述符是谁？服务器/客户端

                //代表是服务器fd有了事件发生：客户端连入事件
                if (i == serverfd)
                {
                    //接收这个客户端的连接
                    clifd = accept(serverfd, NULL, NULL);
                    printf("new client fd = %d\n", clifd);

                    //并把这个客户端的描述符加入到表中
                    FD_SET(clifd, &rdfds);

                    //更新maxfd
                    if (clifd > maxfd)
                    {
                        maxfd = clifd;
                    }
                }
                else //代表客户端有数据
                {
                    bzero(buf, N);
                    int len = recv(i, buf, N, 0);
                    if (len < 0)
                    {
                        perror("recv err");
                        break;
                    }
                    else if (len == 0)
                    {
                        //客户端退出，应该删掉表里的描述符
                        printf("client %d quit\n", i);
                        FD_CLR(i, &rdfds);
                        close(i);
                        break;
                    }
                    else
                    {
                        // printf("recv from %d client = %s\n", i, buf);
                        cJSON *root = cJSON_Parse(buf);
                        cJSON *data = cJSON_GetObjectItem(root, "data");
                        // 灯的状态
                        cJSON *light = cJSON_GetArrayItem(data, 0);
                        cJSON *t = cJSON_GetObjectItem(light, "val");
                        int l = t->valueint;
                        // 空调的状态
                        cJSON *kt = cJSON_GetArrayItem(data, 1);
                        t = cJSON_GetObjectItem(light, "val");
                        int k = t->valueint;
                        // 加湿器状态
                        cJSON *js = cJSON_GetArrayItem(data, 2);
                        t = cJSON_GetObjectItem(js, "val");
                        int j = t->valueint;
                        // 温度
                        cJSON *temp = cJSON_GetArrayItem(data, 3);
                        t = cJSON_GetObjectItem(temp, "val");
                        double te = t->valuedouble;
                        // 湿度
                        cJSON *humi = cJSON_GetArrayItem(data, 4);
                        t = cJSON_GetObjectItem(humi, "val");
                        double h = t->valuedouble;
                        // 电量
                        cJSON *dl1 = cJSON_GetArrayItem(data, 5);
                        t = cJSON_GetObjectItem(dl1, "val");
                        double d111 = t->valuedouble;

                        // 从共享内从中找到相应的结构体
                        struct shm_param para;
                        if (shm_init(&para, "shm", 1024) < 0)
                        {
                            perror("shm_init error");
                            return -1;
                        }
                        int *m = shm_getaddr(&para);
                        if (m < 0)
                        {
                            perror("shm_getaddr error");
                            return -1;
                        }
                        st = (struct std_node *)(m + 1);

                        for (int i = 0; i < *m; i++)
                        {
                            // j1表示加湿器
                            if (st->key == j1)
                                st->new_val.i_val = j;
                            else if (st->key == l1) // l1 表示灯
                                st->new_val.i_val = l;
                            else if (st->key == k1) // k1表示空调
                                st->new_val.i_val = k;
                            else if (st->key == t1) // t1表示湿度
                                st->new_val.f_val = h;
                            else if (st->key == d1) // d1表示电量
                            {
                                st->new_val.f_val = d111;
                                // printf("%f\n",st->new_val.f_val);

                            }
                            else if (st->key == t2) // t2表示温度
                                st->new_val.f_val = te;
                            st += 1;
                        }
                    }
                }
            }
        }
    }
    // 取共享内存打印测试

    close(clifd);
    close(serverfd);
    fclose(fp);
    pthread_detach(tid);
    return 0;
}