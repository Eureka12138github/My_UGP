#ifndef STORAGE_H
#define STORAGE_H
#include "stm32f10x.h"
#include "bsp_flash.h"

extern u16 Store_Data[];
void Store_Init(void);
void Store_Save(void);
void Store_Clear(void);
#endif
