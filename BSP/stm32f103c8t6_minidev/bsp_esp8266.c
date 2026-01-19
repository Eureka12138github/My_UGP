// ESP8266 硬件驱动
#include "bsp_esp8266.h"
/**
 * @brief  初始化USART2串口,	TX-PA2		RX-PA3
 * @param  baud: 串口波特率
 * @retval 无
 */
void BSP_ESP8266_Init(unsigned int baud)
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

