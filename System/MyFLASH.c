#include "stm32f10x.h"
//地址以000、400、800、C00结尾的都是页起始地址
u32 MyFLASH_ReadWord(u32 Address)//读取字
{
	return *((__IO u32 *)(Address));//__IO等同于volatile,是防止编译器优化的
}

u16 MyFLASH_ReadHalfWord(u32 Address)//读取半字
{
	return *((__IO u16 *)(Address));//__IO等同于volatile,是防止编译器优化的
}

u8 MyFLASH_ReadByte(u32 Address)//读取字节
{
	return *((volatile u8 *)(Address));//先将地址强转为指针，再读取指针数据
}

void MyFLASH_EraseAllPages(void)//全擦除会把程序也会擦除掉，可用于防拆的安全保障
{
	FLASH_Unlock();//先进行FLASH解锁
	FLASH_EraseAllPages();//全擦除
	FLASH_Lock();//锁定FLASH防止误操作
	
}

void MyFLASH_ErasePage(u32 Page_Address)
{
	FLASH_Unlock();
	FLASH_ErasePage(Page_Address);	
	FLASH_Lock();
}

void MyFLASH_ProgramWord(u32 Address,u32 Data)//指定地址写入一个字
{
	FLASH_Unlock();
	FLASH_ProgramWord(Address,Data);
	FLASH_Lock();
}
	
void MyFLASH_ProgramHalfWord(u32 Address,u16 Data)//指定地址写入一个半字
{
	FLASH_Unlock();
	FLASH_ProgramHalfWord(Address,Data);
	FLASH_Lock();
}



