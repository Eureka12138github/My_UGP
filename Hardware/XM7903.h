#ifndef __XM7903_H
#define __XM7903_H
#include "stm32f10x.h"                  // Device header
#include "CRC16.h"
#include "usart.h"
#define TxPaket_Length		8
float Get_Nosie_Data(void);
void XM7903_SendPacket(void);

#endif 
