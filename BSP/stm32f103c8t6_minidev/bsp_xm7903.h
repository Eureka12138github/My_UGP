#ifndef _BSP_XM7903_H_
#define _BSP_XM7903_H_
#include "stm32f10x.h"
#include "bsp_usart.h"
//C库
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#define USART_DEBUG		USART3		//调试打印所使用的串口组

void BSP_XM7903_Init(void);
void BSP_XM7903_SendQuery(void);
void BSP_XM7903_StartReceive(void);          
const uint8_t* BSP_XM7903_GetRxBuffer(void); 
extern volatile bool xm7903_rx_ready;



#endif
