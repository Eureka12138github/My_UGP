#include "AD.h"
__align(4) uint16_t raw_adc;  // 内存4字节对齐
void AD_Init(void)
{
    /* 时钟配置 */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1 | RCC_APB2Periph_GPIOA, ENABLE);
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
    RCC_ADCCLKConfig(RCC_PCLK2_Div6);  // ADCCLK=12MHz

    /* GPIO配置（仅PA0）*/
    GPIO_InitTypeDef GPIO_InitStruct = {
        .GPIO_Mode = GPIO_Mode_AIN,
        .GPIO_Pin = GPIO_Pin_5,        // 仅保留通道0对应引脚
        .GPIO_Speed = GPIO_Speed_50MHz
    };
    GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* 定时器配置（保持20kHz触发）*/
    TIM_TimeBaseInitTypeDef TIM_InitStruct;
    TIM_InitStruct.TIM_Prescaler = 35;        // 72MHz/(35+1)=2MHz
    TIM_InitStruct.TIM_Period = 99;           // 2.18MHz/(99+1)=20kHz
    TIM_InitStruct.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_InitStruct.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseInit(TIM3, &TIM_InitStruct);
    TIM_SelectOutputTrigger(TIM3, TIM_TRGOSource_Update);
    TIM_Cmd(TIM3, ENABLE);

    /* ADC配置（单通道模式）*/
    ADC_RegularChannelConfig(ADC1, ADC_Channel_5, 1, ADC_SampleTime_13Cycles5);

    ADC_InitTypeDef ADC_InitStruct = {
        .ADC_Mode = ADC_Mode_Independent,
        .ADC_ScanConvMode = DISABLE,          // 禁用扫描模式
        .ADC_ContinuousConvMode = DISABLE,
        .ADC_ExternalTrigConv = ADC_ExternalTrigConv_T3_TRGO,
        .ADC_DataAlign = ADC_DataAlign_Right,
        .ADC_NbrOfChannel = 1                // 仅1个通道
    };
    ADC_Init(ADC1, &ADC_InitStruct);
    ADC_ExternalTrigConvCmd(ADC1, ENABLE);

    /* DMA配置（单数据循环模式）*/
    DMA_InitTypeDef DMA_InitStruct = {
        .DMA_PeripheralBaseAddr = (uint32_t)&ADC1->DR,
        .DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord,
        .DMA_PeripheralInc = DMA_PeripheralInc_Disable,
        .DMA_MemoryBaseAddr = (uint32_t)&raw_adc,  // 存储到变量
        .DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord,
        .DMA_MemoryInc = DMA_MemoryInc_Disable,     // 禁用地址递增
        .DMA_DIR = DMA_DIR_PeripheralSRC,
        .DMA_BufferSize = 1,               // 每次传输1个数据
        .DMA_Mode = DMA_Mode_Circular,
        .DMA_M2M = DMA_M2M_Disable,
        .DMA_Priority = DMA_Priority_High
    };
    DMA_Init(DMA1_Channel1, &DMA_InitStruct);
    DMA_Cmd(DMA1_Channel1, ENABLE);
// 在DMA初始化后添加中断配置
	DMA_ITConfig(DMA1_Channel1, DMA_IT_TC, ENABLE);
	NVIC_EnableIRQ(DMA1_Channel1_IRQn);
    /* 校准ADC */
	ADC_Cmd(ADC1, ENABLE);                     // 先上电ADC
	ADC_ResetCalibration(ADC1);                // 校准流程
	while(ADC_GetResetCalibrationStatus(ADC1));
	ADC_StartCalibration(ADC1);
	while(ADC_GetCalibrationStatus(ADC1));
	ADC_DMACmd(ADC1, ENABLE);                  // 后启用DMA
}

// 中断服务函数（示例）
volatile bool dma_C11_flag = false;
void DMA1_Channel1_IRQHandler(void) {
	if(DMA_GetITStatus(DMA1_IT_TC1)) {
		DMA_ClearITPendingBit(DMA1_IT_TC1);
		dma_C11_flag = true;  // 通知主循环数据已就绪
		}
}
