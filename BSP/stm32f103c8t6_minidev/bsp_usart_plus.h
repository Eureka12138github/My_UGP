#ifndef __BSP_USART_PLUS_H
#define __BSP_USART_PLUS_H
#include "stm32f10x.h"                  // Device header
#include "cbuf_slot.h" 
#include <stdio.h>
#include <stdarg.h>

// ====== 用户配置区 ======
#define SERIAL_USE_USART1   0
#define SERIAL_USE_USART2   0
#define SERIAL_USE_USART3   1

#define TX_BUF_SIZE         128
#define RX_BUF_SIZE         64

#define USART_DEBUG         USART3   // printf 重定向目标
// =======================

// 函数声明（带条件编译）
#if SERIAL_USE_USART1
void Usart1_Init(uint32_t baud);
#endif
#if SERIAL_USE_USART2
void Usart2_Init(uint32_t baud);
#endif
#if SERIAL_USE_USART3
void Usart3_Init(uint32_t baud);
#endif

// 通用 API（始终可用）
int Serial_SendByte(USART_TypeDef* USARTx, uint8_t Byte);
size_t Serial_SendArray(USART_TypeDef* USARTx, const uint8_t* Array, uint16_t Length);
size_t Serial_SendString(USART_TypeDef* USARTx, const char* String);
void Serial_SendNum(USART_TypeDef* USARTx, uint32_t Num, uint8_t Length);
void Serial_Printf(USART_TypeDef* USARTx, char* format, ...);

bool Serial_Available(USART_TypeDef* USARTx);
int Serial_ReadByte(USART_TypeDef* USARTx);
size_t Serial_ReadArray(USART_TypeDef* USARTx, uint8_t* buf, size_t len);
size_t Serial_GetRxCount(USART_TypeDef* USARTx);

#endif

//以空行结尾
