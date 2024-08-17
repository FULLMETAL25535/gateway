#include "head.h"

char msg[1024] = {0};
char *addr = NULL;
int port = 0;

volatile MQTTClient_deliveryToken deliveredtoken;
extern MQTTClient client;
extern MQTTClient_message pubmsg;
extern MQTTClient_deliveryToken token;
// extern struct temp *tp;

void delivered(void *context, MQTTClient_deliveryToken dt)
{
    printf("Message with token value %d delivery confirmed\n", dt);
    deliveredtoken = dt;
}
int type = 0;
int period = 0;
int msgarrvd(void *context, char *topicName, int topicLen, MQTTClient_message *message)
{
    int i;
    char *payloadptr;
    memset(msg, 0, sizeof(msg));
    printf("Message arrived,topic: %s\n",topicName);

    payloadptr = message->payload;
    for (i = 0; i < message->payloadlen; i++)
    {
        msg[i] = *payloadptr++;
        // putchar(*payloadptr++);
    }
    // putchar('\n');
    msg[i] = '\n';
    //对接收到的mqtt解析并发送对应的消息队列
    parsemqttjson(msg);
    MQTTClient_freeMessage(&message);
    MQTTClient_free(topicName);
    return 1;
}

void connlost(void *context, char *cause)
{
    printf("\nConnection lost\n");
    printf("cause: %s\n", cause);
}

FILE *IOinit()
{
    FILE *fp = (FILE *)malloc(sizeof(FILE *));
    struct stat st;
    do
    {
        int res = stat("/mnt/config/node.json", &st);
        if (res == -1)
        {
            sleep(1);
            continue;
        }
    } while (st.st_size <100); //判断当前点表文件中是否存在点表
    fp = fopen("/mnt/config/node.json", "r+");
    if (fp == NULL)
    {
        printf("fopen err\n");
    }

    return fp;
}

int parsejson(char *buff) //解析点表并往内存中写结构体
{
    cJSON *object = cJSON_Parse(buff);
    if (!cJSON_HasObjectItem(object, "stm32") || !cJSON_HasObjectItem(object, "modbus"))
    {
        printf("点表中无stm32节点或无modbus节点\n");
        return -1;
    }
    cJSON *item;
    cJSON *items;
    //获取点表中的MQTT端口号和IP地址并按格式存入ADDRESS<"tcp://192.168.50.79:1883">
    cJSON *mqtt = cJSON_GetObjectItem(object, "mqtt_server");
    addr = cJSON_GetObjectItem(mqtt, "addr")->valuestring;
    port = cJSON_GetObjectItem(mqtt, "port")->valueint;
    sprintf(ADDRESS, "tcp://%s:%d", addr, port);

    cJSON *stm32 = cJSON_GetObjectItem(object, "stm32");
    cJSON *modbus = cJSON_GetObjectItem(object, "modbus"); //获取点表中两个数据采集的节点
    cJSON *stm32array = cJSON_GetObjectItem(stm32, "data");
    cJSON *modbusarray = cJSON_GetObjectItem(modbus, "data"); //获取节点中的data数组
    cJSON *report = cJSON_GetObjectItem(object, "report");
    items = cJSON_GetObjectItem(report, "type");
    type = items->valueint;
    items = cJSON_GetObjectItem(report, "period");
    period = items->valueint;
    struct shm_param para;
    int *p = NULL;
    struct std_node *q = NULL;
    if (shm_init(&para, "shm", 1024) < 0) //初始化共享内存
    {
        printf("共享内存初始化失败\n");
        return -1;
    }
    p = shm_getaddr(&para); //映射并获取共享内存的首地址
    q = (struct std_node *)(p + 1);

    if (*p != 0)
    {
        for (int i = 0; i < cJSON_GetArraySize(stm32array); i++) //读温湿度的key并保存
        {
            item = cJSON_GetArrayItem(stm32array, i);
            items = cJSON_GetObjectItem(item, "name");
            if (items == NULL)
            {
                printf("stm32中不存在name\n");
                return -1;
            }
            if (!strcmp("temp", items->valuestring))
            {
                tp.tempkey = (q + i)->key;
            }
            else if (!strcmp("temp1", items->valuestring))
            {
                tp.temp1key = (q + i)->key;
            }
        }
        return 0;
    }

    for (int i = 0; i < cJSON_GetArraySize(stm32array); i++) //写共享内存stm32
    {
        *p += 1;

        item = cJSON_GetArrayItem(stm32array, i);
        items = cJSON_GetObjectItem(item, "key");
        if (items == NULL)
        {
            printf("stm32中不存在key\n");
            return -1;
        }
        q->key = items->valueint;

        items = cJSON_GetObjectItem(item, "type");
        if (items == NULL)
        {
            printf("stm32中不存在type\n");
            return -1;
        }
        q->type = items->valueint;

        q->dev_type = 0;
        item = cJSON_GetArrayItem(stm32array, i);
        items = cJSON_GetObjectItem(item, "name");
        if (items == NULL)
        {
            printf("stm32中不存在name\n");
            return -1;
        }
        if (!strcmp("temp", items->valuestring))
        {
            tp.tempkey = q->key;
        }
        else if (!strcmp("temp1", items->valuestring))
        {
            tp.temp1key = q->key;
        }
        q++;
    }
    for (int i = 0; i < cJSON_GetArraySize(modbusarray); i++) //写共享内存modbus
    {
        *p += 1;
        item = cJSON_GetArrayItem(modbusarray, i);
        items = cJSON_GetObjectItem(item, "key");
        if (items == NULL)
        {
            printf("modbus中不存在key\n");
            return -1;
        }
        q->key = items->valueint;
        items = cJSON_GetObjectItem(item, "type");
        if (items == NULL)
        {
            printf("modbus中不存在type\n");
            return -1;
        }
        q->type = items->valueint;
        q->dev_type = 1;
        q++;
    }

    cJSON_Delete(object);
    return 0;
}

int parsemqttjson(char *buff)
{
    cJSON *object = cJSON_Parse(buff);
    cJSON *item = cJSON_GetObjectItem(object, "type");
    if (item == NULL)
    {
        printf("json中不存在type\n");
        return -1;
    }
    struct shm_param para;
    int *p = NULL;
    if (shm_init(&para, "shm", 1024) < 0) //初始化共享内存
    {
        printf("共享内存初始化失败\n");
        return -1;
    }

    if (item->valueint == 1)
    {
        p = shm_getaddr(&para); //映射并获取共享内存的首地址
        struct std_node *q = (struct std_node *)(p + 1);
        //创建object并插入type和result，赋值
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
        printf("数据打包发送成功。\n");
    }
    else if (item->valueint == 2) //控制
    {
        p = shm_getaddr(&para); //映射并获取共享内存的首地址
        struct std_node *q = (struct std_node *)(p + 1);
        struct std_msg msg;
        //判断当前的控制指令的key是源自哪个设备
        cJSON *array = cJSON_GetObjectItem(object, "data");
        cJSON *items = cJSON_GetObjectItem(array, "key");
        int vb = 1;
        for (int i = 0; i < *p; i++)
        {
            if ((q + i)->key == items->valueint)
            {                               //判断他是哪个设备的 stm32-》long为10 modbus-》long为5
                if ((q + i)->dev_type == 1) //modbus
                {
                    msg.msg_type = 5;
                }
                else if ((q + i)->dev_type == 0) //stm32
                {
                    msg.msg_type = 10;
                }
                msg.key = (q + i)->key;
                items = cJSON_GetObjectItem(array, "val");
                strcpy(msg.val, items->valuestring);
            }
        }
        //将msg发送
        if(msg_queue_send("msg", &msg, sizeof(struct std_msg), 0)==0)
        {
            printf("控制指令发送成功。");
        }else
        {
            printf("控制指令发送失败。");
        }
        
    }
    else if (item->valueint == 3) //上报模式修改
    {
        cJSON *array = cJSON_GetObjectItem(object, "data");
        cJSON *items = cJSON_GetObjectItem(array, "type");
        type = items->valueint;
        items = cJSON_GetObjectItem(array, "period");
        period = items->valueint;

        FILE *fp = IOinit();
        char buffer[3072];
        fread(buffer, 1, sizeof(buffer), fp);
        cJSON *objectdb = cJSON_Parse(buffer);
        cJSON_ReplaceItemInObject(objectdb, "report", array);
        char *db = cJSON_Print(objectdb);
        fclose(fp);

        fp = fopen("/mnt/config/node.json", "w+");
        if(fwrite(db, sizeof(char), strlen(db), fp))
        {
            printf("上报模式修改:%d\n",type);
        }else
        {
            printf("上报模式修改失败");
        }
        

        // cJSON_Delete(objectdb);
        free(db);
        fclose(fp);
        db = NULL;
    }
    else if (item->valueint == 4)
    {
        //解析时间
        cJSON *timearr = cJSON_GetObjectItem(object, "limit");
        long long start_timestamp = cJSON_GetArrayItem(timearr, 0)->valuedouble;
        long long end_timestamp = cJSON_GetArrayItem(timearr, 1)->valuedouble;
        //读数据库数据,进回调函数处理
        sqlite3 *dbs;
        char *errmsg = 0;

        int ret = sqlite3_open("history.db", &dbs); //打开数据库
        if (ret != SQLITE_OK)
        {
            printf("Can't open database: %s\n", sqlite3_errmsg(dbs));
            sqlite3_close(dbs);
            return 1;
        }
        //创建根节点和数组节点
        cJSON *hisobject = cJSON_CreateObject();
        cJSON_AddItemToObject(hisobject, "type", cJSON_CreateNumber(4));
        cJSON *hisarray = cJSON_CreateArray();
        //根据时间进行数据查询
        char sql_select[256];
        sqlite3_stmt *stmt;
        sprintf(sql_select, "SELECT * FROM history WHERE time >= %lld AND time <= %lld;", start_timestamp, end_timestamp);
        ret = sqlite3_prepare_v2(dbs, sql_select, -1, &stmt, 0);
        if (ret != SQLITE_OK)
        {
            fprintf(stderr, "Prepare error: %s\n", sqlite3_errmsg(dbs));
            sqlite3_close(dbs);
            return 1;
        }
        //查询到的数据进行序列化
        while (sqlite3_step(stmt) == SQLITE_ROW)
        {
            long long time = sqlite3_column_int(stmt, 0);
            float temp = sqlite3_column_int(stmt, 1);
            float temp1 = sqlite3_column_int(stmt, 2);
            //每一组数据插入数组节点
            cJSON *obarr = cJSON_CreateObject();
            cJSON_AddItemToObject(obarr, "time", cJSON_CreateNumber(time));
            char str[32] = {0};
            char str1[32] = {0};
            sprintf(str, "%.2f", temp);
            cJSON_AddItemToObject(obarr, "temp", cJSON_CreateString(str));
            sprintf(str1, "%.2f", temp1);
            cJSON_AddItemToObject(obarr, "temp1", cJSON_CreateString(str1));
            cJSON_AddItemToArray(hisarray, obarr);
        }
        //数组节点插入跟节点
        cJSON_AddItemToObject(hisobject, "data", hisarray);
        char *ss = cJSON_Print(hisobject);

        //发送
        pubmsg.payload = ss;
        pubmsg.payloadlen = (int)strlen(ss);
        pubmsg.qos = QOS;
        pubmsg.retained = 0;
        MQTTClient_publishMessage(client, TOPIC_UP, &pubmsg, &token);
        printf("历史数据上报成功");
        //释放
        sqlite3_finalize(stmt);
        sqlite3_close(dbs);
        cJSON_Delete(hisobject);
    }
    cJSON_Delete(object);
    return 0;
}

void *handler_thread(void *arg)
{
    sqlite3 *hisdb;
    char *errmsg;
    //打开或创建数据库
    int ret = sqlite3_open("history.db", &hisdb);
    if (ret != SQLITE_OK)
    {
        printf("err open:%s\n", sqlite3_errmsg(hisdb));
        return NULL;
    }
    //打开或创建温度湿度历史表
    char create_table_sql[128] = "CREATE TABLE history("
                                 "time     INTEGER  PRIMARY KEY,"
                                 "temp     INTEGER  ,"
                                 "temp1    INTEGER  );";
    ret = sqlite3_exec(hisdb, create_table_sql, NULL, NULL, &errmsg);
    if (ret != SQLITE_OK)
    {
        printf("table: %s\n", errmsg);
    }

    struct shm_param para; //打开共享内存
    int *p = NULL;
    struct std_node *q = NULL;
    if (shm_init(&para, "shm", 1024) < 0) //初始化共享内存
    {
        printf("共享内存初始化失败\n");
    }
    p = shm_getaddr(&para); //映射并获取共享内存的首地址
    q = (struct std_node *)(p + 1);
    float temp, temp1;
    printf("开始存储历史数据");
    //这里进行循环
    while (1)
    {
        for (int i = 0; i < *p; i++) //将共享内存中的所有数据读取并创建节点插入data数组中
        {
            if (tp.tempkey == (q + i)->key) //找到temp(湿度)的部分
            {
                //获取temp数据
                temp = (q + i)->new_val.f_val;
            }
            else if (tp.temp1key == (q + i)->key)
            {
                //获取temp数据
                temp1 = (q + i)->new_val.f_val;
            }
        }

        //往数据库中存数
        long long time = getCurrentSeconds(); //获取时间
        char sql_insert[256];
        sprintf(sql_insert, "INSERT INTO history (time, temp, temp1) VALUES ('%lld', '%.2f', '%.2f');", time, temp, temp1);
        int rc = sqlite3_exec(hisdb, sql_insert, NULL, NULL, NULL);
        if (rc != SQLITE_OK)
        {
            fprintf(stderr, "insert error: %s\n", sqlite3_errmsg(hisdb));
        }

        sleep(1);
    }

    return NULL;
}

long long getCurrentSeconds()
{
    time_t current_time;
    time(&current_time);
    return (long long)current_time;
}
