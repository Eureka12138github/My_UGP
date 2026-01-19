// PMS7003 硬件驱动
#include "bsp_pms7003.h"
// PMS7003 数据接收流程说明：
// 1. 调用 BSP_PMS7003_Init() 完成 USART、DMA 及 GPIO 初始化；
// 2. 传感器数据通过 DMA 自动存入 s_RxBuffer（32 字节循环缓冲区）；
// 3. 每次 DMA 接收完整一帧后，置位 pms7003_rx_ready 标志；
// 4. 上层可在检测到该标志后，读取缓冲区并进行协议解析与校验。
//
// 本文件仅负责硬件初始化与数据接收，不涉及协议解析。


static uint8_t s_RxBuffer[PMS7003_PACKET_LEN] __attribute__((aligned(4)));// DMA 接收缓冲区
volatile bool pms7003_rx_ready = false;//接收完成标志位

/**
 * @brief 返回已经接收32字节数据完毕的只读数组
 */
const uint8_t* BSP_PMS7003_GetRxBuffer(void) {
    return s_RxBuffer;
}

/**
 * @brief 初始化 PMS7003 传感器通信接口
 * @details 配置 USARTx 与 DMA 接收功能，用于获取 PMS7003 空气质量传感器数据
 * @note 
 * - 遵循 PMS7003 协议：32字节数据包格式，包含起始符(0x42,0x4D)、28字节数据、2字节校验
 * - 使用DMA循环模式实现自动连续接收，防止数据丢失
 * - DMA接收完成后触发传输完成中断，在中断服务程序中处理数据
 * - NVIC优先级分组应在main()函数中统一配置，此处不再重复配置
 */
void BSP_PMS7003_Init(void) 
{
    /* 1. 统一开启所有所需外设时钟 */
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);                                    // 开启DMA1时钟（AHB总线）
    RCC_APB2PeriphClockCmd(PMS7003_USART_CLK |                                           // 开启USART时钟（APB2总线）
                           PMS7003_UART_GPIO_CLK |                                       // 开启UART GPIO时钟（APB2总线）
                           PMS7003_CTRL_GPIO_CLK, ENABLE);                               // 开启控制GPIO时钟（APB2总线）

    /* 2. 配置PMS7003控制引脚：PB5(PM_RESET)、PB6(PM_SET) */
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Pin = PMS7003_RST_PIN | PMS7003_EN_PIN;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(PMS7003_CTRL_GPIO_PORT, &GPIO_InitStructure);
    GPIO_SetBits(PMS7003_CTRL_GPIO_PORT, PMS7003_RST_PIN | PMS7003_EN_PIN); // 初始化为高电平

    /* 3. 配置USART通信引脚 */
    // TX引脚配置：复用推挽输出，连接传感器RX引脚
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;      // 复用推挽输出
    GPIO_InitStructure.GPIO_Pin = PMS7003_TX_PIN;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;    // 高速模式保证信号质量
    GPIO_Init(PMS7003_UART_GPIO_PORT, &GPIO_InitStructure);
    
    // RX引脚配置：上拉输入，连接传感器TX引脚
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;        // 上拉输入模式
    GPIO_InitStructure.GPIO_Pin = PMS7003_RX_PIN;
    GPIO_Init(PMS7003_UART_GPIO_PORT, &GPIO_InitStructure);

    /* 4. 配置USART参数（严格遵循PMS7003通信协议） */
    USART_InitTypeDef USART_InitStructure;
    USART_InitStructure.USART_BaudRate = PMS7003_BAUDRATE;                              // 波特率：9600
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;                         // 8位数据位
    USART_InitStructure.USART_StopBits = USART_StopBits_1;                              // 1位停止位
    USART_InitStructure.USART_Parity = USART_Parity_No;                                 // 无奇偶校验
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;                     // 收发双工模式
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;     // 无硬件流控
    USART_Init(PMS7003_USART, &USART_InitStructure);

    /* 5. 配置DMA接收（使用DMA1通道5对应USART_RX） */
    DMA_InitTypeDef DMA_InitStruct;
    DMA_StructInit(&DMA_InitStruct);  // 初始化结构体为默认值
    
    DMA_InitStruct.DMA_PeripheralBaseAddr = (uint32_t)&PMS7003_USART->DR;               // 外设地址：USART数据寄存器
    DMA_InitStruct.DMA_MemoryBaseAddr = (uint32_t)s_RxBuffer;                      // 内存地址：接收缓冲区
    DMA_InitStruct.DMA_DIR = DMA_DIR_PeripheralSRC;                                     // 传输方向：外设到内存
    DMA_InitStruct.DMA_BufferSize = PMS7003_PACKET_LEN;                                 // 缓冲区大小：32字节（协议规定）
    DMA_InitStruct.DMA_Mode = DMA_Mode_Circular;                                        // 循环模式：自动重置缓冲区指针
    DMA_InitStruct.DMA_Priority = DMA_Priority_High;                                    // 高优先级确保实时性
    DMA_InitStruct.DMA_PeripheralInc = DMA_PeripheralInc_Disable;                       // 外设地址固定不变
    DMA_InitStruct.DMA_MemoryInc = DMA_MemoryInc_Enable;                                // 内存地址递增
    DMA_InitStruct.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;                // 外设数据宽度：字节
    DMA_InitStruct.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;                        // 内存数据宽度：字节
    
    DMA_Init(PMS7003_DMA_CHANNEL, &DMA_InitStruct);                                     // 初始化DMA通道

    /* 6. 配置DMA中断优先级（基于系统已设定的优先级分组） */
    // 注意：NVIC优先级分组应在main()函数中统一配置，此处仅设置具体中断的优先级
    NVIC_InitTypeDef NVIC_DMA_InitStructure;
    NVIC_DMA_InitStructure.NVIC_IRQChannel = PMS7003_DMA_IRQ;                          // DMA中断号
    NVIC_DMA_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;                       // 抢占优先级：最高
    NVIC_DMA_InitStructure.NVIC_IRQChannelSubPriority = 1;                              // 子优先级：较高
    NVIC_DMA_InitStructure.NVIC_IRQChannelCmd = ENABLE;                                 // 使能中断通道
    NVIC_Init(&NVIC_DMA_InitStructure);

    /* 7. 启动DMA传输链路（最后启动，确保所有配置已完成） */
    USART_DMACmd(PMS7003_USART, USART_DMAReq_Rx, ENABLE);                               // 使能USART的DMA接收请求
    USART_Cmd(PMS7003_USART, ENABLE);                                                   // 使能USART（必须在DMA配置完成后）
    DMA_Cmd(PMS7003_DMA_CHANNEL, ENABLE);                                               // 启动DMA通道
    DMA_ITConfig(PMS7003_DMA_CHANNEL, DMA_IT_TC, ENABLE);                               // 使能传输完成中断
}

/**
 * @brief DMA1通道5传输完成中断服务函数
 * @details 处理PMS7003传感器数据接收完成事件，当DMA接收到完整的32字节数据包时触发此中断
 * @note 
 * - 该中断由DMA传输完成事件触发，表示已接收到一个完整的PMS7003数据包
 * - 中断标志必须及时清除，避免重复进入中断服务程序
 * - 设置全局标志位dma_C15_flag通知主程序有新数据到达
 * - 此中断处理程序应尽可能简洁，避免在中断中进行复杂的数据处理
 */
void DMA1_Channel5_IRQHandler(void)
{
    // 检查DMA传输完成中断状态标志
    if (DMA_GetITStatus(DMA1_IT_TC5)) {
        // 清除DMA传输完成中断标志位（必须清除，否则会持续触发中断）
        DMA_ClearITPendingBit(DMA1_IT_TC5);
        
        // 设置DMA传输完成标志位，通知主程序有新的32字节数据包到达
        // 该标志位可用于主程序轮询检测或触发进一步的数据处理流程
        pms7003_rx_ready = true;  
    }
}
