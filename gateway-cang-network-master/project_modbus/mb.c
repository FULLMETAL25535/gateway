#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mb.h"
#include "cJSON.h"
#include "list.h"

// 从 JSON 文件中解析 modbus 数据并插入链表
void parse_and_insert_modbus_data(const char *json_str, struct list_head *mb_list) 
{
    // 解析 JSON 字符串
    cJSON *root = cJSON_Parse(json_str);
    if (root == NULL) 
    {
        printf("Error parsing JSON\n");
        return;
    }

    //获取MODBUS IP和PORT
    cJSON *mb_dev = cJSON_GetObjectItem(root, "mb_dev");
    if (mb_dev == NULL) 
    {
        printf("No mb_dev data found\n");
        cJSON_Delete(root);
        return;
    }

    cJSON *addr = cJSON_GetObjectItem(mb_dev, "addr");
    cJSON *port_item = cJSON_GetObjectItem(mb_dev, "port");

    strcpy(SERVER_IP,addr->valuestring);    
    SERVER_PORT = port_item->valueint;

    printf("SERVER_IP: %s\n", SERVER_IP);
    printf("SERVER_PORT: %d\n", SERVER_PORT);

    // 获取 modbus 数据
    cJSON *modbus = cJSON_GetObjectItem(root, "modbus");
    if (modbus == NULL) 
    {
        printf("No modbus data found\n");
        cJSON_Delete(root);
        return;
    }

    // 获取 modbus 数据数组
    cJSON *data_array = cJSON_GetObjectItem(modbus, "data");

    // 遍历 modbus 数据数组
    cJSON *data_item;
    cJSON_ArrayForEach(data_item, data_array) 
    {
        struct mb_node_list *new_node = (struct mb_node_list *)malloc(sizeof(struct mb_node_list));
        if (new_node == NULL) 
        {
            printf("Memory allocation failed\n");
            cJSON_Delete(root);
            return;
        }

        // 提取数据项并赋值给新节点
        cJSON *key = cJSON_GetObjectItem(data_item, "key");
        cJSON *name = cJSON_GetObjectItem(data_item, "name");
        cJSON *type = cJSON_GetObjectItem(data_item, "type");
        cJSON *addr = cJSON_GetObjectItem(data_item, "addr");

        if (key != NULL) new_node->node.key = key->valueint;
        if (name != NULL) strncpy(new_node->node.name, name->valuestring, sizeof(new_node->node.name) - 1);
        if (type != NULL) new_node->node.type = type->valueint;
        if (addr != NULL) new_node->node.addr = addr->valueint;

        // 初始化链表节点并插入链表
        INIT_LIST_HEAD(&new_node->list);
        list_add_tail(&new_node->list, mb_list);
    }

    // 释放 JSON 解析结果
    cJSON_Delete(root);
}