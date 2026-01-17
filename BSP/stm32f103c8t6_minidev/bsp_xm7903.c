//硬件驱动
// bsp_xm7903.c
#include "bsp_xm7903.h"

#define XM7903_RESPONSE_LEN   7   // MODBUS 响应固定7字节
static uint8_t s_RxBuffer[XM7903_RESPONSE_LEN] __attribute__((aligned(4)));
volatile bool xm7903_rx_ready = false;

void BSP_XM7903_Init(void)
{
    // 1. 时钟使能
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE); // DMA 时钟

    // 2. GPIO 配置（PB10=TX, PB11=RX）
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING; // 或 IPD，避免上拉干扰
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    // 3. USART 配置
    USART_InitTypeDef USART_InitStructure;
    USART_InitStructure.USART_BaudRate = 9600;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
    USART_Init(USART3, &USART_InitStructure);

    // 4. DMA 配置（仅配置，不启动！）
    DMA_InitTypeDef DMA_InitStructure;
    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&(USART3->DR);
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
    DMA_Init(DMA1_Channel3, &DMA_InitStructure);         // USART3_RX → DMA1_CH3

    // 注意：这里不调用 DMA_Cmd(ENABLE)！

    // 5. 使能 USART 的 DMA 请求（但 DMA 本身未启动）
    USART_DMACmd(USART3, USART_DMAReq_Rx, ENABLE);

    // 6. 使能 USART3
    USART_Cmd(USART3, ENABLE);

    // 7. 配置 DMA 传输完成中断（但初始不触发）
    DMA_ITConfig(DMA1_Channel3, DMA_IT_TC, ENABLE);

    NVIC_InitTypeDef NVIC_InitStructure;
    NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel3_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}


// 查询帧固定，可静态定义
static const uint8_t query_frame[] = {0x01, 0x03, 0x00, 0x00, 0x00, 0x01, 0x84, 0x0A};

void BSP_XM7903_SendQuery(void) {
    for (int i = 0; i < sizeof(query_frame); i++) {
        while (!USART_GetFlagStatus(USART3, USART_FLAG_TXE));
        USART_SendData(USART3, query_frame[i]);
    }
}

// 新增函数：启动一次 7 字节 DMA 接收
void BSP_XM7903_StartReceive(void)
{
    // 安全：确保 DMA 已停止
    DMA_Cmd(DMA1_Channel3, DISABLE);
    
    // 重载计数器（虽然长度不变，但规范做法）
    DMA_SetCurrDataCounter(DMA1_Channel3, XM7903_RESPONSE_LEN);
    
    // 清除可能残留的中断标志（防误触发）
    DMA_ClearITPendingBit(DMA1_IT_TC3);
    
    // 启动 DMA 接收
    DMA_Cmd(DMA1_Channel3, ENABLE);
}

const uint8_t* BSP_XM7903_GetRxBuffer(void)
{
    return s_RxBuffer;
}

// 中断服务函数（不变）
void DMA1_Channel3_IRQHandler(void)
{
    if (DMA_GetITStatus(DMA1_IT_TC3)) {
        DMA_ClearITPendingBit(DMA1_IT_TC3);
        DMA_Cmd(DMA1_Channel3, DISABLE); // 自动关闭
        xm7903_rx_ready = true;
    }
}

