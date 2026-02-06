#ifndef ONENET_MQTT_H
#define ONENET_MQTT_H

//单片机头文件
#include "stm32f10x.h"

//网络设备
#include "esp8266_drv.h"

//协议文件
#include "mqtt_kit.h"

//算法
#include "base64.h"
#include "hmac_sha1.h"

//硬件驱动
#include "bsp_usart.h"
#include "bsp_delay.h"

#include "cJSON.h"
#include "storage.h"//FLASH

//C库
#include <string.h>
#include <stdio.h>
#include "stdbool.h"

//OneNet配置文件
#include "system_config.h"

#include "debug_config.h"
#include "storage.h"

#define ONENET_OK                0   // 协议处理成功（无论业务结果）
#define ONENET_PARSE_ERR         1   // MQTT 或 JSON 解析失败
#define ONENET_POST_SUCCESS      2   // 收到 post/reply 且成功
#define ONENET_POST_FAILED       3   // 收到 post/reply 但失败
#define ONENET_SET_HANDLED       4   // 成功处理了 set 指令（可选）



unsigned char OneNet_DevLink(void);

int8_t OneNet_SendData(void);

void OneNET_Subscribe(void);

u8 OneNet_RevPro(unsigned char *cmd);

#endif
