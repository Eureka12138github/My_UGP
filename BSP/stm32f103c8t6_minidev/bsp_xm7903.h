#ifndef _BSP_XM7903_H_
#define _BSP_XM7903_H_
#include "bsp_config.h"  // 包含配置头文件

void BSP_XM7903_Init(void);
void BSP_XM7903_SendQuery(void);
void BSP_XM7903_StartReceive(void);          
const uint8_t* BSP_XM7903_GetRxBuffer(void); 
extern volatile bool xm7903_rx_ready;



#endif
