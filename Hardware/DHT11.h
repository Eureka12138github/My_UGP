#ifndef __DHT11_H
#define __DHT11_H
#include "stm32f10x.h"
// Device header
#define Pin_Select GPIO_Pin_12
#define DHT11_High GPIO_SetBits(GPIOB, Pin_Select)
#define DHT11_Low GPIO_ResetBits(GPIOB, Pin_Select)
#define DHT11_DQ GPIO_ReadInputDataBit(GPIOB, Pin_Select)
u8 DHT11_Init(void);
u8 DHT11_Read_Data(u16 *temp,u16 *humi);
u8 DHT11_Check(void);
void DHT11_Rst(void);

#endif 
