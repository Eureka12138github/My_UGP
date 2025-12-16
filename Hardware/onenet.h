#ifndef _ONENET_H_
#define _ONENET_H_

//单片机头文件
#include "stm32f10x.h"

//网络设备
#include "esp8266.h"

//协议文件
#include "mqttkit.h"

//算法
#include "base64.h"
#include "hmac_sha1.h"

//硬件驱动
#include "usart.h"
#include "Delay.h"
#include "led.h"

#include "cJSON.h"
#include "Store.h"//FLASH

//C库
#include <string.h>
#include <stdio.h>
#include "stdbool.h"

//OneNet配置文件
#include "project_secrets.h"

_Bool OneNET_RegisterDevice(void);

unsigned char OneNet_DevLink(void);

int8_t OneNet_SendData(void);

void OneNET_Subscribe(void);

u8 OneNet_RevPro(unsigned char *cmd);

#endif
