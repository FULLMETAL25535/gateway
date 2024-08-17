#include "pub_define.h"
#include "MQTTClient.h"
#include "cJSON.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/msg.h>
#include <errno.h>
#include <fcntl.h>
#define ADDRESS "tcp://localhost:1883"
#define CLIENTID "ExampleClientSub"
#define TOPICDOWN "/app/data/down"
#define TOPICUP "/app/data/up"
#define QOS 1
#define TIMEOUT 10000L
volatile MQTTClient_deliveryToken deliveredtoken;
void data_sub_pub(char*msg);
int msgarrvd(void *context, char *topicName, int topicLen, MQTTClient_message *message);