#include <stdio.h>
#include <pthread.h>
#include <modbus.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <errno.h>
#include <string.h>
#include "shmem.h"
#include "msg_queue_peer.h"

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

char c,d;
modbus_t *ctx;
struct msg//共享空间结构体
{
    int flag;
    char buf[32];
};

struct msgbuf//消息队列结构体
{
    long type; //必须用long，表示消息的类型，值>0
    //正文随便定义
    int a;
    int b;
};

void shmgett(uint16_t *q)//创建共享内存
{
    struct msg *p;

    struct shm_param para;
    // shm_init
    if(shm_init(&para, "test", 1024) < 0)
    {
        printf("shm init err\n");
        return ;
    }

    p = shm_getaddr(&para);
    if(p == NULL)
    {
        printf("shm get addr err\n");
        return ;
    }

   sprintf(p->buf,"%d %d %d %d",q[0],q[1],q[2],q[3]);

   printf("%s\n",p->buf);
}

//创建消息队列
void msggett()
{
    //读取消息
    struct msgbuf m;

    while(1)
    {
        msg_queue_recv("msg", &m, sizeof(m), 10, 0);
        // msgrcv(msgid, &m, sizeof(m) - sizeof(long), 10, 0);//10:表示读取消息的类型为10,0：代表阻塞，读取完消息才返回
        // c = (char )m.a;
        // d = (char )m.b;
        char buf[32]={};
        printf("mingling:%d %d\n", m.a,m.b);
        sprintf(buf,"%d %d",m.a,m.b);
        modbus_write_bit(ctx, m.a, m.b);
    }
    
}

void *abc_thread(void *arg)
{
    modbus_t *ctx = (modbus_t *)arg;
    uint16_t dest[20]={0};
    
    while(1)
    {
        modbus_read_registers(ctx, 0, 4, dest);
        for(int i=0;i<4;i++)
        {
            printf("%#x ",dest[i]);
        }
        printf("\n");
        //创建共享内存
        shmgett(dest);
        
        sleep(1);
    }
    return NULL; 
}

int main(int argc, char const *argv[])
{
    ctx = modbus_new_tcp("192.168.50.229", 502);
    //设置从机id 
    modbus_set_slave(ctx, 1);
    
    //建立连接
    modbus_connect(ctx);
    //创建线程
    pthread_t tid;
    if(pthread_create(&tid,NULL,abc_thread,ctx) != 0)
    {
        perror("create thread err");
        return -1;
    } 
    //强制线圈状态05
    uint8_t src[10];
    char buf[3];
    msggett();//打开消息队列
    
    pthread_detach(tid);

    //关闭套接字
    modbus_close(ctx);
    //释放实例
    modbus_free(ctx);
    
    return 0;
}
