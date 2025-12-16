#ifndef __MYIWD_H
#define __MYIWD_H
#include "stm32f10x.h"                  // Device header
#include "OLED_UI.h"
#include "Delay.h"
#include "Store.h"
#include "ErrorWarningLog.h"
extern u16 Reset_Count;
void MYIWD_Init(uint16_t MaxTime);
void Check_Reset_Way(void);



#endif
