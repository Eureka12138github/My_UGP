//XM7903 硬件驱动
#include "bsp_xm7903.h"
// XM7903 数据接收流程说明：
// 1. 调用 BSP_XM7903_Init() 完成 USART、DMA、GPIO 及中断初始化；
// 2. 主程序调用 BSP_XM7903_SendQuery() 发送 MODBUS 查询帧；
// 3. 紧接着调用 BSP_XM7903_StartReceive() 启动一次 7 字节 DMA 接收；
// 4. DMA 接收完成后触发中断，置位 xm7903_rx_ready 标志；
// 5. 上层检测到标志后，通过 BSP_XM7903_GetRxBuffer() 获取响应数据。
//
// 本文件仅负责硬件通信与数据接收，不包含 MODBUS 协议解析。

#define XM7903_RESPONSE_LEN   7   // MODBUS 响应固定7字节

static uint8_t s_RxBuffer[XM7903_RESPONSE_LEN] __attribute__((aligned(4)));
volatile bool xm7903_rx_ready = false;

/**
 * @brief 初始化XM7903传感器硬件接口
 * 
 * 配置USART串口通信、DMA接收功能以及相关中断，
 * 为后续的数据收发做好硬件准备
 */
void BSP_XM7903_Init(void)
{
    // 1. 时钟使能
    RCC_APB1PeriphClockCmd(XM7903_USART_CLK, ENABLE);
    RCC_APB2PeriphClockCmd(XM7903_GPIO_CLK, ENABLE);
    RCC_AHBPeriphClockCmd(XM7903_DMA_CLK, ENABLE); // DMA 时钟

    // 2. GPIO 配置（PB10=TX, PB11=RX）
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin = XM7903_TX_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(XM7903_GPIO_PORT, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = XM7903_RX_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING; // 或 IPD，避免上拉干扰
    GPIO_Init(XM7903_GPIO_PORT, &GPIO_InitStructure);

    // 3. USART 配置
    USART_InitTypeDef USART_InitStructure;
    USART_InitStructure.USART_BaudRate = XM7903_BAUDRATE;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
    USART_Init(XM7903_USART, &USART_InitStructure);

    // 4. DMA 配置（仅配置，不启动！）
    DMA_InitTypeDef DMA_InitStructure;
    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&(XM7903_USART->DR);
    DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)s_RxBuffer;
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
    DMA_InitStructure.DMA_BufferSize = XM7903_RESPONSE_LEN; // 7字节
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
    DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;        // ← 关键：Normal 模式！
    DMA_InitStructure.DMA_Priority = DMA_Priority_Medium;
    DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
    DMA_Init(XM7903_DMA_CHANNEL, &DMA_InitStructure);    // 使用宏替换通道

    // 注意：这里不调用 DMA_Cmd(ENABLE)！

    // 5. 使能 USART 的 DMA 请求（但 DMA 本身未启动）
    USART_DMACmd(XM7903_USART, USART_DMAReq_Rx, ENABLE);

    // 6. 使能 USART
    USART_Cmd(XM7903_USART, ENABLE);

    // 7. 配置 DMA 传输完成中断（但初始不触发）
    DMA_ITConfig(XM7903_DMA_CHANNEL, DMA_IT_TC, ENABLE);

    NVIC_InitTypeDef NVIC_InitStructure;
    NVIC_InitStructure.NVIC_IRQChannel = XM7903_DMA_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}


// 查询帧固定，可静态定义
static const uint8_t query_frame[] = {0x01, 0x03, 0x00, 0x00, 0x00, 0x01, 0x84, 0x0A};

/**
 * @brief 发送MODBUS查询命令到XM7903传感器
 * 
 * 通过USART逐字节发送预定义的查询帧，实现对传感器数据的请求
 */
void BSP_XM7903_SendQuery(void) {
    for (int i = 0; i < sizeof(query_frame); i++) {
        while (!USART_GetFlagStatus(XM7903_USART, USART_FLAG_TXE));
        USART_SendData(XM7903_USART, query_frame[i]);
    }
}

/**
 * @brief 启动一次7字节的DMA接收操作
 * 
 * 重新配置并启动DMA接收，用于接收传感器响应数据
 * 此函数确保在每次接收前正确初始化DMA状态
 */
void BSP_XM7903_StartReceive(void)
{
    // 安全：确保 DMA 已停止
    DMA_Cmd(XM7903_DMA_CHANNEL, DISABLE);
    
    // 重载计数器（虽然长度不变，但规范做法）
    DMA_SetCurrDataCounter(XM7903_DMA_CHANNEL, XM7903_RESPONSE_LEN);
    
    // 清除可能残留的中断标志（防误触发）
    DMA_ClearITPendingBit(XM7903_DMA_TC_IT);
    
    // 启动 DMA 接收
    DMA_Cmd(XM7903_DMA_CHANNEL, ENABLE);
}

/**
 * @brief 获取接收到的传感器数据缓冲区指针
 * 
 * @return const uint8_t* 指向接收缓冲区的常量指针
 *         包含最近一次从传感器接收的7字节MODBUS响应数据
 */
const uint8_t* BSP_XM7903_GetRxBuffer(void)
{
    return s_RxBuffer;
}

/**
 * @brief DMA传输完成中断服务函数
 * 
 * 在DMA完成7字节数据接收后被调用，负责清理DMA状态
 * 并设置接收完成标志位，通知主程序数据已就绪
 */
void DMA1_Channel3_IRQHandler(void)
{
    if (DMA_GetITStatus(XM7903_DMA_TC_IT)) {
        DMA_ClearITPendingBit(XM7903_DMA_TC_IT);
        DMA_Cmd(XM7903_DMA_CHANNEL, DISABLE); // 自动关闭
        xm7903_rx_ready = true;
    }
}

