#ifndef __ESP8266_DRV_H
#define __ESP8266_DRV_H
#include "stm32f10x.h"                  // Device header
#include "bsp_usart.h" 					//串口通信支持
#include "bsp_config.h"					//硬件端口配置，波特率等
#include "debug_config.h"				//调试控制
#include "project_secrets.h"			//WIFI信息，设备名称密钥等
#include "Delay.h"						//SysTick 系统滴答支持
#include "OLED.h"						//初始化进度文字显示

//初始化，所用串口，波特率多少，复位引脚是否要拉高
void ESP8266_HardwareInit(void);
_Bool ESP8266_SendCmd(const char *cmd, const char *res);
u8 ESP8266_Init(void);
//...


#endif 
