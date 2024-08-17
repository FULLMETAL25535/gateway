gcc modbus2.c cJSON.c shmem.c msg_queue_peer.c -lmodbus -lpthread -lm

消息队列格式
union val_t
{ 
    int b_val;  //bool类型存储空间，控制扫地机器人，窗帘
    int i_val;   //整形值存储空间
    float f_val;  //浮点值存储空间
};

struct std_node
{
	long msg_type;  //消息类型
	int key;        //唯一键值
    int dev_type;  //数据点属于哪个设备，根据网关支持的设备自行定义
	union val_t val;  //需要修改成的数据
};
共享内容格式

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