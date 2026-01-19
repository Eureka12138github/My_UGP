#ifndef _BSP_ESP8266_H_
#define _BSP_ESP8266_H_


#include "stm32f10x.h"
#include <stdbool.h>
#include "delay.h"     

//C库
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
unsigned char* ESP8266_ParseIPD(void);


void BSP_ESP8266_Init(unsigned int baud);



#endif
