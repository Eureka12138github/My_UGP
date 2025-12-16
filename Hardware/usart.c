/**
	************************************************************
	************************************************************
	************************************************************
	*	文件名： 	usart.c
	*
	*	作者： 		张继瑞
	*
	*	日期： 		2016-11-23
	*
	*	版本： 		V1.0
	*
	*	说明： 		单片机串口外设初始化，格式化打印
	*
	*	修改记录：	
	************************************************************
	************************************************************
	************************************************************
**/

//硬件驱动
#include "usart.h"
#include "delay.h"

//C库
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

uint8_t Usart3_TxPaket[TxPaket_LEN];
__attribute__((aligned(4))) uint8_t Serial_RxPacket[PMS_PACKET_LEN];
uint8_t Usart3_RxPaket[7];

uint8_t Serial_RxFlag = 0;

/**
 * @brief 初始化USART1并配置DMA接收PMS7003传感器数据
 * @param baud 波特率，根据PMS7003协议应设置为9600
 * @note 
 * - 遵循PMS7003协议：32字节数据包，包含2起始符(0x42,0x4D)、28数据字节、2校验字节
 * - 使用DMA循环模式实现自动连续接收，避免数据覆盖
 * - DMA接收完成后触发传输完成中断，需配合中断服务函数处理数据
 */
void Usart1_Init(unsigned int baud) {
    /* 1. 启用DMA1时钟（关键步骤！AHB总线上的DMA控制器需要独立时钟） */
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
    
    /* 2. 启用外设时钟（USART1在APB2总线，GPIOA在APB2总线） */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1 | RCC_APB2Periph_GPIOA, ENABLE);

    /* 3. 配置GPIO（根据传感器接口定义） */
    GPIO_InitTypeDef GPIO_InitStructure;
    // PA9作为 USART1_TX ：复用推挽输出，连接传感器RX（需接1KΩ电阻做电平匹配）
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;  // 复用推挽输出
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; // 高速模式确保信号完整性
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    // PA10作为USART1_RX：上拉输入，连接传感器TX（抗干扰设计）
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;     // 上拉输入模式
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    /* 4. 配置USART1参数（严格匹配传感器协议） */
    USART_InitTypeDef USART_InitStructure;
    USART_InitStructure.USART_BaudRate = baud;       // 必须为9600（规格书规定）
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;  // 8位数据位
    USART_InitStructure.USART_StopBits = USART_StopBits_1;     // 1位停止位
    USART_InitStructure.USART_Parity = USART_Parity_No;        // 无校验位
    USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx; // 双工模式
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None; // 无流控
    USART_Init(USART1, &USART_InitStructure);
    USART_Cmd(USART1, ENABLE);  // 使能USART1

    /* 5. 配置DMA接收（通道5映射USART1_RX） */
    DMA_InitTypeDef DMA_InitStruct;
    DMA_StructInit(&DMA_InitStruct);  // 初始化结构体为默认值
    // 外设地址：USART1数据寄存器（DR寄存器地址需强制转换）
    DMA_InitStruct.DMA_PeripheralBaseAddr = (uint32_t)&USART1->DR; 
    // 内存地址：接收缓冲区（需4字节对齐，防DMA传输异常）
    DMA_InitStruct.DMA_MemoryBaseAddr = (uint32_t)Serial_RxPacket;  
    DMA_InitStruct.DMA_DIR = DMA_DIR_PeripheralSRC;    // 传输方向：外设→内存
    DMA_InitStruct.DMA_BufferSize = PMS_PACKET_LEN;   // 数据包长度32字节（协议规定）
    DMA_InitStruct.DMA_Mode = DMA_Mode_Circular;       // 循环模式：自动重置缓冲区
    DMA_InitStruct.DMA_Priority = DMA_Priority_High;   // 高优先级确保实时性
    DMA_InitStruct.DMA_PeripheralInc = DMA_PeripheralInc_Disable; // 外设地址固定
    DMA_InitStruct.DMA_MemoryInc = DMA_MemoryInc_Enable;          // 内存地址自增
    DMA_InitStruct.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte; // 字节传输
    DMA_InitStruct.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
    DMA_Init(DMA1_Channel5, &DMA_InitStruct);  // 初始化DMA1通道5

    /* 6. 使能DMA传输链 */
    USART_DMACmd(USART1, USART_DMAReq_Rx, ENABLE); // 使能USART1的DMA接收请求
    DMA_Cmd(DMA1_Channel5, ENABLE);               // 启动DMA通道
    DMA_ITConfig(DMA1_Channel5, DMA_IT_TC, ENABLE); // 使能传输完成中断

    /* 7. 配置中断优先级（遵循CMSIS标准） */
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2); // 设置优先级分组（2位抢占，2位响应）
    NVIC_InitTypeDef NVIC_DMA_InitStructure;
    NVIC_DMA_InitStructure.NVIC_IRQChannel = DMA1_Channel5_IRQn; // 通道5中断
    NVIC_DMA_InitStructure.NVIC_IRQChannelPreemptionPriority = 1; // 最高抢占优先级
    NVIC_DMA_InitStructure.NVIC_IRQChannelSubPriority = 1;        // 子优先级
    NVIC_DMA_InitStructure.NVIC_IRQChannelCmd = ENABLE;          // 使能中断
    NVIC_Init(&NVIC_DMA_InitStructure);
    NVIC_EnableIRQ(DMA1_Channel5_IRQn);  // 最后显式启用中断（避免优先级配置未完成时触发）
}

/**
 * @brief  初始化USART2串口,	TX-PA2		RX-PA3
 * @param  baud: 串口波特率
 * @retval 无
 */
void Usart2_Init(unsigned int baud)
{
    // 定义GPIO初始化结构体
    GPIO_InitTypeDef gpio_initstruct;
    // 定义USART初始化结构体
    USART_InitTypeDef usart_initstruct;
    // 定义NVIC初始化结构体
    NVIC_InitTypeDef nvic_initstruct;
    
    // 使能GPIOA和USART2的时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
    
    // PA2 TXD 配置为复用推挽输出
    gpio_initstruct.GPIO_Mode = GPIO_Mode_AF_PP;
    gpio_initstruct.GPIO_Pin = GPIO_Pin_2;
    gpio_initstruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &gpio_initstruct);
   

	//与EXP8266复位有关
	gpio_initstruct.GPIO_Mode = GPIO_Mode_Out_PP;				//设置为输出
	gpio_initstruct.GPIO_Pin = GPIO_Pin_4;						//将初始化的Pin脚
	gpio_initstruct.GPIO_Speed = GPIO_Speed_50MHz;				//可承载的最大频率
	GPIO_Init(GPIOA, &gpio_initstruct);	
	GPIO_SetBits(GPIOA,GPIO_Pin_4);
	
    // PA3 RXD 配置为浮空输入
    gpio_initstruct.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    gpio_initstruct.GPIO_Pin = GPIO_Pin_3;
    gpio_initstruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &gpio_initstruct);
    
    // 配置USART2参数
    usart_initstruct.USART_BaudRate = baud;
    usart_initstruct.USART_HardwareFlowControl = USART_HardwareFlowControl_None;    // 无硬件流控
    usart_initstruct.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;                    // 接收和发送模式
    usart_initstruct.USART_Parity = USART_Parity_No;                                // 无校验位
    usart_initstruct.USART_StopBits = USART_StopBits_1;                              // 1位停止位
    usart_initstruct.USART_WordLength = USART_WordLength_8b;                        // 8位数据位
    USART_Init(USART2, &usart_initstruct);
    
    // 使能USART2
    USART_Cmd(USART2, ENABLE);                                                     
    
    // 使能USART2接收中断
    USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);                                  
    // 可选：使能空闲中断
    // USART_ITConfig(USART2, USART_IT_IDLE, ENABLE);  
	
    
    // 配置NVIC中断控制器
    nvic_initstruct.NVIC_IRQChannel = USART2_IRQn;
    nvic_initstruct.NVIC_IRQChannelCmd = ENABLE;
    nvic_initstruct.NVIC_IRQChannelPreemptionPriority = 0;
    nvic_initstruct.NVIC_IRQChannelSubPriority = 0;
    NVIC_Init(&nvic_initstruct);
}
//如果可以的话，或许可以尝试想串口1那样配置为DMA传输，今日就到此吧。250419
void Usart3_Init(unsigned int baud)
{
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);	//开启USART1的时钟
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);	//开启GPIOA的时钟
	
	/*GPIO初始化*/
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);					//将PB10引脚初始化为复用推挽输出
	
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);					//将PB11引脚初始化为上拉输入
	
	/*USART初始化*/
	USART_InitTypeDef USART_InitStructure;					//定义结构体变量
	USART_InitStructure.USART_BaudRate = baud;				//波特率
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;	//硬件流控制，不需要
	USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;	//模式，发送模式和接收模式均选择
	USART_InitStructure.USART_Parity = USART_Parity_No;		//奇偶校验，不需要
	USART_InitStructure.USART_StopBits = USART_StopBits_1;	//停止位，选择1位
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;		//字长，选择8位
	USART_Init(USART3, &USART_InitStructure);				//将结构体变量交给USART_Init，配置USART1
	
	/*中断输出配置*/
	USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);			//开启串口接收数据的中断
	
	/*NVIC中断分组*/
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);			//配置NVIC为分组2
	
	/*NVIC配置*/
	NVIC_InitTypeDef NVIC_InitStructure;					//定义结构体变量
	NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;		//选择配置NVIC的USART1线
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//指定NVIC线路使能
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;		//指定NVIC线路的抢占优先级为1
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;		//指定NVIC线路的响应优先级为1
	NVIC_Init(&NVIC_InitStructure);							//将结构体变量交给NVIC_Init，配置NVIC外设
	
	/*USART使能*/
	USART_Cmd(USART3, ENABLE);	
}
	/*开启时钟*/



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

/**
 * @brief 通过USART发送一个字节的数据
 * 
 * 本函数用于通过指定的USART接口发送一个字节的数据。它首先将数据字节写入USART的发送数据寄存器(TDR)，
 * 然后等待直到数据被实际发送出去。这个等待过程是通过检查USART的TXE标志来实现的，TXE标志表示发送数据寄存器是否为空。
 * 
 * @param USARTx 指向USART外设的指针，用于指定使用哪个USART接口发送数据
 * @param Byte 要发送的数据字节
 */
void Serial_SendByte(USART_TypeDef *USARTx,uint8_t Byte)
{
    USART_SendData(USARTx,Byte);//此函数将Byte变量写入到TDR
    while(USART_GetFlagStatus(USART3,USART_FLAG_TXE)==RESET);//等待TDR的数据转移到移位寄存器
}

/**
 * @brief 通过串口发送数组
 * 
 * 本函数通过指定的串口外设发送一个数组中的所有字节该函数依赖于Serial_SendByte函数来完成实际的字节发送操作
 * 
 * @param USARTx 指定的串口外设实例，例如USART1, USART2等
 * @param Array 指向待发送的数组的指针
 * @param Length 待发送数组的长度
 */
void Serial_SendArray(USART_TypeDef *USARTx,uint8_t*Array,uint16_t Length)       
{
    // 初始化数组索引
    uint16_t i;
    // 遍历数组中的每个元素
    for(i=0;i<Length;i++)
    {
        // 调用Serial_SendByte函数发送当前元素
        Serial_SendByte(USARTx,Array[i]);//发送一个数组
    }
}

/**
 * @brief 通过串口发送字符串
 * 
 * 本函数负责将一个字符串通过指定的串口USARTx发送出去。它利用了字符串的结束标志位'\0'来确定字符串的长度，
 * 因此不需要额外的长度参数。函数内部通过循环逐个字符发送，确保整个字符串被完整发送。
 * 
 * @param USARTx 指定的串口外设，例如USART1, USART2等。它决定了字符串从哪个串口发送。
 * @param String 指向待发送字符串的指针。字符串必须是以'\0'结束的，符合C语言标准的字符串。
 */
void Serial_SendString(USART_TypeDef *USARTx,char*String)//字符串自带结束标志位故无需传递长度参数
{
    uint8_t i;
    // 循环遍历字符串中的每个字符，直到遇到结束标志位'\0'。
    for(i=0;String[i]!='\0';i++)
    {
        // 将String字符串中的每个字符通过Serial_SendByte函数发送。这里体现了字符串发送的逐字节处理方式。
        Serial_SendByte(USARTx,String[i]);//将String字符串一个个取出，通过SendByte发送
    }
}

/**
 * @brief 计算x的y次幂
 * 
 * 该函数使用循环计算一个数的幂次。它接受两个uint32_t类型的参数：
 * 基数x和指数y。函数通过将基数x连续乘以自身y次来计算结果。
 * 
 * @param x 基数
 * @param y 指数
 * @return uint32_t 计算得到的幂次结果
 */
uint32_t Serial_Pow(uint32_t x,uint32_t y)
{
    // 初始化结果为1，因为任何数的0次幂都是1
    uint32_t Result=1;
    
    // 循环y次，每次都将Result乘以x
    while(y--)
    {
        Result*=x;
    }
    
    // 返回计算得到的幂次结果
    return Result;
}

/**
 * @brief 通过串口发送一个整数
 * 
 * 该函数将一个整数按照指定的长度拆解成单个数字，并通过串口逐一发送
 * 每个数字会被转换成对应的ASCII码后发送
 * 
 * @param USARTx 串口实例，比如USART1, USART2等
 * @param Num 待发送的整数
 * @param Length 整数的位数，决定发送的字节数
 */
void Serial_SendNum(USART_TypeDef *USARTx,uint32_t Num,uint8_t Length)
{
    uint8_t i;
    // 循环发送每一位数字
    for(i=0;i<Length;i++)
    {
        // 拆解整数的每一位，转换为ASCII码后发送
        // 通过除以10的幂来获取每一位数字，再通过取模操作得到当前位的数字，最后加上0x30转换为ASCII码
        Serial_SendByte(USARTx,Num/Serial_Pow(10,Length-i-1)%10+0x30);
    }
}
/**
 * @brief 通过串口发送数据包
 * 
 * 该函数负责组装并发送一个数据包，数据包由包头、数据部分和包尾组成
 * 发送数据包的过程包括三个步骤：
 * 1. 发送包头，使用0xFF作为标识
 * 2. 发送数据包本身，这里假定数据包已经存储在Serial_TxPaket中，且长度为4
 * 3. 发送包尾，使用0xFE作为标识
 * 
 * @param USARTx 指定的串口外设实例，例如USART1、USART2等
 */
void Serial_SendPacket(USART_TypeDef *USARTx)
{
//    Serial_SendByte(USARTx,0xFF);//发送包头
    Serial_SendArray(USARTx,Usart3_TxPaket,TxPaket_LEN);//发送数据包
//    Serial_SendByte(USARTx,0xFE);//发送包尾
}

/*
//不粘包用这个：
uint8_t pms_buffer[PMS_PACKET_LEN];//（同时在头文件加上：extern uint8_t pms_buffer[];），之后在主函数调用此数组即可
uint8_t Serial_GetRxFlag(void)//此函数用于判断是否接收到了数据包
{
	if(Serial_RxFlag==1){
	 __disable_irq();          
	// 2. 快速拷贝数据到处理缓冲区
	memcpy(pms_buffer, Serial_RxPaket, sizeof(Serial_RxPaket));          
	// 3. 清除标志并启用中断
	__enable_irq();
	Serial_RxFlag=0;
	return 1;
	}
	return 0;
}	
//但在现在的传感器数据包读取的应用中，我认为粘包没有太大影响，因为在串口中断中已经判断过校验位了，
//故还是用原来的版本
*/
/**
 * 此函数用于判断是否接收到了数据包
 * 
 * 当Serial_RxFlag标志位为1时，表示已接收到数据包
 * 在函数内部，标志位会被重置，以备下次接收数据
 * 
 * @return uint8_t 返回1表示已接收到数据包，0表示没有接收到
 */
uint8_t Serial_GetRxFlag(void)
{
    // 检查是否接收到数据包
    if(Serial_RxFlag==1){
        // 重置接收标志位，准备下一次接收
        Serial_RxFlag=0;
        // 表示已接收到数据包
        return 1;
    }
    // 没有接收到数据包
    return 0;
}


/**
 * @brief USART1中断服务例程(这是原始版本，使用串口中断，后经过改进，使用DMA版本，该版本仍觉得保留，日后可能用得上)250309
 * 
 * 本函数用于处理USART1的中断请求，主要负责串口数据的接收。
 * 接收过程采用状态机方式，以处理数据包的同步、数据接收和校验。
 */
//void USART1_IRQHandler(void)
//{
//    // 定义状态机状态，用于跟踪接收过程的不同阶段
//    static uint8_t RxState = 0;    
//    // 定义数据包索引，用于指向当前接收的数据包位置
//    static uint8_t pRxPacket = 0;   
//    // 定义校验和计算变量，用于累加接收到的数据以进行校验
//    static uint16_t checksum = 0;   

//    // 检查USART1的接收数据寄存器非空中断状态
//    if(USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)
//    {
//        // 读取接收到的数据
//        uint8_t RxData = USART_ReceiveData(USART1);
//        
//        // 根据状态机状态执行相应的操作
//        switch(RxState)
//        {
//        case 0: // 等待第一个起始符
//            // 如果接收到预期的起始符，开始计算校验和，并进入下一状态
//            if(RxData == 0x42) {
//                checksum = RxData;  
//                Serial_RxPaket[pRxPacket++] = RxData;
//                RxState = 1;
//            }
//            break;
//            
//        case 1: // 等待第二个起始符
//            // 如果接收到第二个起始符，继续计算校验和，否则重置状态机
//            if(RxData == 0x4D) {
//                checksum += RxData;
//                Serial_RxPaket[pRxPacket++] = RxData;
//                RxState = 2;
//            } else {
//                RxState = 0;  
//                pRxPacket = 0;
//            }
//            break;
//            
//        case 2: // 接收数据帧（28字节）
//            // 在达到数据包长度限制前，持续接收数据并计算校验和
//            if(pRxPacket < PMS_PACKET_LEN) {
//                Serial_RxPaket[pRxPacket++] = RxData;
//                checksum += RxData;  
//                
//                // 接收完所有数据后进入校验状态
//                if(pRxPacket == PMS_PACKET_LEN) {
//                    RxState = 3;
//                }
//            }
//            break;
//            
//        case 3: // 校验和验证
//        {
//            // 提取接收到的校验和（最后两个字节）
//             uint16_t receivedChecksum = (Serial_RxPaket[PMS_PACKET_LEN-2] << 8) | Serial_RxPaket[PMS_PACKET_LEN-1];
//            // 计算校验和时需排除最后两个校验字节
//             checksum -= (Serial_RxPaket[PMS_PACKET_LEN-2] + Serial_RxPaket[PMS_PACKET_LEN-1]);
//            
//            // 比较计算得到的校验和与接收到的校验和是否一致
//            if(checksum == receivedChecksum) {
//                Serial_RxFlag = 1;  // 校验通过
//            }
//            
//            // 重置状态机以准备下一次接收
//            RxState = 0;
//            pRxPacket = 0;
//            checksum = 0;
//            break;
//        }
//            
//        default:
//            // 如果状态机进入未知状态，重置状态机
//            RxState = 0;
//            pRxPacket = 0;
//            break;
//        }
//        
//        // 清除USART1的接收数据寄存器非空中断标志
//        USART_ClearITPendingBit(USART1, USART_IT_RXNE);
//    }
//}



/**
 * @brief DMA1通道5中断服务例程
 * 
 * 本函数用于处理DMA1通道5的中断请求，主要负责在DMA传输完成后进行数据校验。
 * 校验过程包括检查起始符是否符合协议要求、计算并验证校验和。
 */
volatile bool dma_C15_flag = false;
void DMA1_Channel5_IRQHandler(void)
{
    // 检查DMA1通道5传输完成中断状态
    if (DMA_GetITStatus(DMA1_IT_TC5)) {
        // 清除DMA1通道5传输完成中断标志
        DMA_ClearITPendingBit(DMA1_IT_TC5);

        // 检查接收到的数据包起始符是否符合协议要求（0x42 0x4D）
        if (Serial_RxPacket[0] == 0x42 && Serial_RxPacket[1] == 0x4D) {
            // 计算前30字节的累加值作为校验和
            uint16_t checksum = 0;
            for (int i = 0; i < PMS_PACKET_LEN - 2; i++) {
                checksum += Serial_RxPacket[i];
            }
            // 提取接收到的校验和（最后2字节，大端序）
            uint16_t receivedChecksum = (Serial_RxPacket[PMS_PACKET_LEN-2] << 8) | Serial_RxPacket[PMS_PACKET_LEN-1];
            
            // 验证计算得到的校验和与接收到的校验和是否一致
            if (checksum == receivedChecksum && checksum != 0) {
                // 如果校验通过，设置标志位以通知主程序处理数据
                dma_C15_flag = true;  
            }
        }
    }
}

void USART3_IRQHandler(void)
{
    static uint8_t pRxPacket = 0;  // 接收缓冲区索引
    
    if (USART_GetITStatus(USART3, USART_IT_RXNE) == SET)
    {
        uint8_t RxData = USART_ReceiveData(USART3);  // 读取接收到的字节
        
        /* 直接存储数据到缓冲区（需确保缓冲区足够大） */
        if (pRxPacket < 7)    // 防止缓冲区溢出
        {
            Usart3_RxPaket[pRxPacket] = RxData;    // 存储数据
            pRxPacket++;                            // 索引自增
        }
        if(pRxPacket == 7)
        {
            pRxPacket = 0;  // 溢出时重置索引（可选：设置错误标志）
			Serial_RxFlag = 1;   // 实时通知有新数据
			
        }
        
        /* 每次接收完成一个字节即置标志位（或根据需求调整） */
        
        USART_ClearITPendingBit(USART3, USART_IT_RXNE);  // 清除中断标志
    }
}
