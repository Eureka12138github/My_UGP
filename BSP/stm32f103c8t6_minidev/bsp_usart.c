//硬件驱动
#include "bsp_usart.h"
#include "delay.h"     

//C库
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>


/**
 * @brief  初始化USART2串口,	TX-PA2		RX-PA3
 * @param  baud: 串口波特率
 * @retval 无
 */
void Usart2_Init(unsigned int baud)
{
    // 定义GPIO初始化结构体
    GPIO_InitTypeDef GPIO_InitStructure;
    // 定义USART初始化结构体
    USART_InitTypeDef USART_InitStructure;
    // 定义NVIC初始化结构体
    NVIC_InitTypeDef NVIC_InitStructure;
    
    // 使能GPIOA和USART2的时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
    
    // PA2 TXD 配置为复用推挽输出
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
   

	//与EXP8266复位有关
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;				//设置为输出
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;						//将初始化的Pin脚
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;				//可承载的最大频率
	GPIO_Init(GPIOA, &GPIO_InitStructure);	
	GPIO_SetBits(GPIOA,GPIO_Pin_4);
	
    // PA3 RXD 配置为浮空输入
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    // 配置USART2参数
    USART_InitStructure.USART_BaudRate = baud;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;    // 无硬件流控
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;                    // 接收和发送模式
    USART_InitStructure.USART_Parity = USART_Parity_No;                                // 无校验位
    USART_InitStructure.USART_StopBits = USART_StopBits_1;                              // 1位停止位
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;                        // 8位数据位
    USART_Init(USART2, &USART_InitStructure);
    
    // 使能USART2
    USART_Cmd(USART2, ENABLE);                                                     
    
    // 使能USART2接收中断
    USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);                                  
    // 可选：使能空闲中断
    // USART_ITConfig(USART2, USART_IT_IDLE, ENABLE);  
	
    
    // 配置NVIC中断控制器
    NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_Init(&NVIC_InitStructure);
}




/**
 * @brief 通过USART发送字符串
 * 
 * 本函数通过指定的USART接口发送一个字符串。它依次发送字符串中的每个字符，
 * 并在每个字符发送完成后才发送下一个字符，确保整个字符串被正确发送。
 * 
 * @param USARTx 指定的USART接口，例如USART1、USART2等。
 * @param str 指向要发送的字符串的指针。
 * @param len 要发送的字符串的长度。
 */
void Usart_SendString(USART_TypeDef *USARTx, unsigned char *str, unsigned short len)
{
    // 初始化计数器
    unsigned short count = 0;
    USART_ClearFlag(USARTx, USART_FLAG_TC);  // 清除TC标志的初始状态
    // 遍历字符串中的每个字符
    for(; count < len; count++)
    {
        // 发送当前字符
        USART_SendData(USARTx, *str++);									//发送数据
        
        // 等待发送完成，USART_FLAG_TC表示传输完成标志
        while(USART_GetFlagStatus(USARTx, USART_FLAG_TC) == RESET);		//等待发送完成
    }
}

/**
 * @brief 通过USART发送数据
 * 
 * 本函数通过USART实现串行通信，将格式化的数据发送出去
 * 
 * @param USARTx 指定的USART端口，例如USART1、USART2等
 * @param fmt 格式化字符串的格式说明符
 * @param ... 可变参数列表，包含格式化字符串中的具体值
 */
void UsartPrintf(USART_TypeDef *USARTx, char *fmt,...)
{

    // 定义一个296字节的缓冲区用于存储格式化后的字符串
    unsigned char UsartPrintfBuf[296];
    // 定义一个可变参数列表
    va_list ap;
    // 指向缓冲区的指针，用于遍历缓冲区中的字符
    unsigned char *pStr = UsartPrintfBuf;
    
    // 初始化可变参数列表
    va_start(ap, fmt);
    // 根据格式化字符串和可变参数列表，将数据格式化到缓冲区中
    vsnprintf((char *)UsartPrintfBuf, sizeof(UsartPrintfBuf), fmt, ap);							//格式化
    // 结束可变参数列表的使用
    va_end(ap);
    USART_ClearFlag(USARTx, USART_FLAG_TC);  // 清除TC标志的初始状态
    // 遍历缓冲区中的每个字符，通过USART发送出去
    while(*pStr != 0)
    {
        // 发送当前字符
        USART_SendData(USARTx, *pStr++);
        // 等待数据发送完成
        while(USART_GetFlagStatus(USARTx, USART_FLAG_TC) == RESET);
    }

}










	







