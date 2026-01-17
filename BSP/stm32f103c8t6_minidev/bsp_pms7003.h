#ifndef _BSP_PMS7003_H_
#define _BSP_PMS7003_H_
#include "stm32f10x.h"
#include <stdbool.h>   
#include "bsp_config.h"

void BSP_PMS7003_Init(void);
const uint8_t* BSP_PMS7003_GetRxBuffer(void);
extern volatile bool pms7003_rx_ready;
#endif
