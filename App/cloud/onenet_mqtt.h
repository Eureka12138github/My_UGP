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
#include "project_secrets.h"

#include "debug_config.h"

_Bool OneNET_RegisterDevice(void);

unsigned char OneNet_DevLink(void);

int8_t OneNet_SendData(void);

void OneNET_Subscribe(void);

u8 OneNet_RevPro(unsigned char *cmd);

#endif
