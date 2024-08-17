#include "head.h"

MQTTClient client;
MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
MQTTClient_message pubmsg = MQTTClient_message_initializer;
MQTTClient_deliveryToken token;
int rc;

extern int type;

extern int period ;
int main(int argc, char *argv[])
{
    FILE *fp = IOinit();
    char buffer[3072];
    fread(buffer, 1, sizeof(buffer), fp); //读取点表
    fclose(fp);
    int res = parsejson(buffer);
    if (res != 0)
    {
        return 0;
    }
    printf("解析并成功存入共享内存\n");
    pthread_t tid;
    if (pthread_create(&tid, NULL, handler_thread, NULL) != 0) //开启线程，线程中打开数据库并保存数据至数据库
    {
        perror("create thread err");
        return -1;
    }
    printf("addr:%s\n",ADDRESS);
    MQTTClient_create(&client, ADDRESS, CLIENTID,
                      MQTTCLIENT_PERSISTENCE_NONE, NULL);
    conn_opts.keepAliveInterval = 20;
    conn_opts.cleansession = 1;

    MQTTClient_setCallbacks(client, NULL, connlost, msgarrvd, delivered);

    if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS)
    {
        printf("Failed to connect, return code %d\n", rc);
        exit(EXIT_FAILURE);
    }
    MQTTClient_subscribe(client, TOPIC_DOWN, QOS);

    struct shm_param para;
    int *p = NULL;
    if (shm_init(&para, "shm", 1024) < 0) //初始化共享内存
    {
        printf("共享内存初始化失败\n");
        return -1;
    }
    p = shm_getaddr(&para); //映射并获取共享内存的首地址

    while (1)
    {
        //通过对上报类型的区分进行数据的读取上报
        if (type == 0) //不上报
        {
            sleep(1);
        }
        else if (type == 1) //变化上报
        {
            struct std_node *q = (struct std_node *)(p + 1);
            cJSON *object = cJSON_CreateObject();
            cJSON *items = cJSON_CreateNumber(1);
            cJSON_AddItemToObject(object, "type", items); //回复指令类型为采集回复
            items = cJSON_CreateNumber(1);
            cJSON_AddItemToObject(object, "result", items); //是否成功默认值失败，读取数据成功后修改为成功
            cJSON *array = cJSON_CreateArray();
            char number[32] = {0};
            for (int i = 0; i < *p; i++) //将共享内存中的所有数据读取并创建节点插入data数组中
            {
                if ((q+i)->type == 2) //将值转化为字符串并同时判断旧值与新值是否相等，如果相等跳出本次循环并进行下一次循环
                {
                    if ((q+i)->new_val.i_val == (q+i)->old_val.i_val)
                    {
                        continue;
                    }else
                    {
                        (q+i)->old_val = (q+i)->new_val;
                        sprintf(number, "%d", (q+i)->new_val.i_val);
                    }
                }
                else if ((q+i)->type == 1)
                {
                    if ((q+i)->new_val.b_val == (q+i)->old_val.b_val)
                    {
                        continue;
                    }else
                    {
                        (q+i)->old_val = (q+i)->new_val;
                        sprintf(number, "%d", (q+i)->new_val.b_val);
                    }
                }
                else if ((q+i)->type == 3)
                {
                    if ((q+i)->new_val.f_val == (q+i)->old_val.f_val)
                    {
                        continue;
                    }else
                    {
                        (q+i)->old_val = (q+i)->new_val;
                        sprintf(number, "%.2f", (q+i)->new_val.f_val);
                    } 
                }
                cJSON *arrobject = cJSON_CreateObject();
                items = cJSON_CreateNumber(q->key);
                cJSON_AddItemToObject(arrobject, "key", items); //插入key节点
                items = cJSON_CreateString(number);

                cJSON_AddItemToObject(arrobject, "val", items); //插入val节点
                cJSON_AddItemToArray(array, arrobject);         //数组中的小根节点插入
            }
            if (cJSON_GetArraySize(array) == 0)
            {
                sleep(1);
                continue;
            }

            cJSON_AddItemToObject(object, "data", array); //数组节点插入跟节点
            items = cJSON_CreateNumber(0);
            cJSON_ReplaceItemInObject(object, "result", items); //改变读取是否成功的值为成功（0）
            char *mg = cJSON_Print(object);

            //发送数据
            pubmsg.payload = mg;
            pubmsg.payloadlen = (int)strlen(mg);
            pubmsg.qos = QOS;
            pubmsg.retained = 0;
            MQTTClient_publishMessage(client, TOPIC_UP, &pubmsg, &token);
        }
        else if (type == 2) //周期上报
        {
            struct std_node *q = (struct std_node *)(p + 1);
            cJSON *object = cJSON_CreateObject();
            cJSON *items = cJSON_CreateNumber(1);
            cJSON_AddItemToObject(object, "type", items); //回复指令类型为采集回复
            items = cJSON_CreateNumber(1);
            cJSON_AddItemToObject(object, "result", items); //是否成功默认值失败，读取数据成功后修改为成功
            cJSON *array = cJSON_CreateArray();
            char number[32] = {0};


            for (int i = 0; i < *p; i++) //将共享内存中的所有数据读取并创建节点插入data数组中
            {
                cJSON *arrobject = cJSON_CreateObject();
                items = cJSON_CreateNumber(q->key);
                cJSON_AddItemToObject(arrobject, "key", items); //插入key节点
                if (q->type == 2)                               //将值转化为字符串
                {
                    sprintf(number, "%d", q->new_val.i_val);
                }
                else if (q->type == 1)
                {
                    sprintf(number, "%d", q->new_val.b_val);
                }
                else if (q->type == 3)
                {
                    sprintf(number, "%.2f", q->new_val.f_val);
                }
                items = cJSON_CreateString(number);
                cJSON_AddItemToObject(arrobject, "val", items); //插入val节点
                cJSON_AddItemToArray(array, arrobject);         //数组中的小根节点插入
                q->old_val = q->new_val;
                q++;
            }
            cJSON_AddItemToObject(object, "data", array); //数组节点插入跟节点
            items = cJSON_CreateNumber(0);
            cJSON_ReplaceItemInObject(object, "result", items); //改变读取是否成功的值为成功（0）
            char *mg = cJSON_Print(object);

            //发送数据
            pubmsg.payload = mg;
            pubmsg.payloadlen = (int)strlen(mg);
            pubmsg.qos = QOS;
            pubmsg.retained = 0;
            MQTTClient_publishMessage(client, TOPIC_UP, &pubmsg, &token);
            sleep(period);
        }
    }

    MQTTClient_unsubscribe(client, TOPIC_DOWN);
    MQTTClient_disconnect(client, 10000);
    MQTTClient_destroy(&client);

    return rc;
}
