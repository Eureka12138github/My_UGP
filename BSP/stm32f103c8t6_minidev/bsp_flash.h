#ifndef BSP_FLASH_H
#define BSP_FLASH_H
#include "stm32f10x.h"
u8 MyFLASH_ReadByte(u32 Address);
u16 MyFLASH_ReadHalfWord(u32 Address);
u32 MyFLASH_ReadWord(u32 Address);
void MyFLASH_ErasePage(u32 Page_Address);
void MyFLASH_EraseAllPages(void);
void MyFLASH_ProgramHalfWord(u32 Address,u16 Data);
void MyFLASH_ProgramWord(u32 Address,u32 Data);

#endif
