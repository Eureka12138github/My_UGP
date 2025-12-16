#ifndef _USART_H_
#define _USART_H_


#include "stm32f10x.h"
#include <stdbool.h>
#include "XM7903.h"
#define USART_DEBUG		USART3		//调试打印所使用的串口组
#define TxPaket_LEN 8
#define PMS_PACKET_LEN 32  // 0x42+0x4D + 28字节数据 + 2字节校验 = 32字节
unsigned char* ESP8266_ParseIPD(void);

void Usart1_Init(unsigned int baud);

void Usart2_Init(unsigned int baud);

void Usart3_Init(unsigned int baud);

void Usart_SendString(USART_TypeDef *USARTx, unsigned char *str, unsigned short len);

void UsartPrintf(USART_TypeDef *USARTx, char *fmt,...);
uint8_t Serial_GetRxFlag(void);
void Serial_SendByte(USART_TypeDef *USARTx,uint8_t Byte);
void Serial_SendArray(USART_TypeDef *USARTx,uint8_t*Array,uint16_t Length);
void Serial_SendString(USART_TypeDef *USARTx,char*String);
void Serial_SendNum(USART_TypeDef *USARTx,uint32_t Num,uint8_t Length);
void Serial_SendPacket(USART_TypeDef *USARTx);

extern uint8_t Usart3_TxPaket[];
extern uint8_t Usart3_RxPaket[];
extern __attribute__((aligned(4))) uint8_t Serial_RxPacket[];
extern volatile bool dma_C15_flag;
#endif
