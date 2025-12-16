#ifndef __MYFLASH_H
#define __MYFLASH_H

u8 MyFLASH_ReadByte(u32 Address);
u16 MyFLASH_ReadHalfWord(u32 Address);
u32 MyFLASH_ReadWord(u32 Address);
void MyFLASH_ErasePage(u32 Page_Address);
void MyFLASH_EraseAllPages(void);
void MyFLASH_ProgramHalfWord(u32 Address,u16 Data);
void MyFLASH_ProgramWord(u32 Address,u32 Data);

#endif
