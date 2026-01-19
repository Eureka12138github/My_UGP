#ifndef __BSP_OLED_H
#define __BSP_OLED_H
#include "bsp_config.h"  // 包含配置头文件
//	oled初始化函数
void OLED_Init(void);
// 设置颜色模式
void OLED_SetColorMode(bool colormode);
//	oled全局刷新函数
void OLED_Update(void);
//	oled局部刷新函数
void OLED_UpdateArea(uint8_t X, uint8_t Y, uint8_t Width, uint8_t Height);
// OLED 设置亮度函数
void OLED_Brightness(int16_t Brightness);




#endif







