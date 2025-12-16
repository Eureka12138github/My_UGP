#ifndef _ESP8266_H_
#define _ESP8266_H_
#include "MyRTC.h"
#include "OLED.h"
#include <string.h>
#include <stdlib.h>




#define REV_OK		0	//接收完成标志
#define REV_WAIT	1	//接收未完成标志


u8 ESP8266_Init(void);
_Bool ESP8266_SNTP_Time(char *cmd, char *res, MYRTC* TimeStructure);
void ESP8266_Clear(void);

_Bool ESP8266_SendCmd(const char *cmd, char *res);

int ESP8266_SendData(unsigned char *data, unsigned short len);

unsigned char *ESP8266_GetIPD(unsigned short timeOut);


#endif
