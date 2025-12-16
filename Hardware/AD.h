#ifndef __AD_H
#define __AD_H
#include "stm32f10x.h"
#include <stdbool.h>
extern __align(4) uint16_t raw_adc;
extern volatile bool dma_C11_flag;
uint16_t AD_GetValue(void);
void AD_Init(void);
uint16_t Get_Average_Data(void);
#endif 
