#ifndef __BSP_UI_INPUT_H
#define __BSP_UI_INPUT_H
/*【如果您需要移植此项目，则需要更改以下函数的实现方式。】 */
#include "stm32f10x.h"                  // Device header                

//获取确认，取消，上，下按键状态的函数(【Q：为什么使用宏定义而不是函数？A：因为这样可以提高效率，减少代码量】)
#define Key_GetEnterStatus()    GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_6)
#define Key_GetBackStatus()     GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_7)
#define Key_GetUpStatus()       GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_0)
#define Key_GetDownStatus()     GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_1)


//按键初始化函数
void Key_Init(void);


#endif
