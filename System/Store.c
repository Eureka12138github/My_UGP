#include "stm32f10x.h"
#include "MyFLASH.h"
#define STORE_START_ADDRESS 0X800FC00
#define STORE_COUNT 512
u16 Store_Data[STORE_COUNT];//512个数据，每个数据16位，2字节，正好对应闪存的一页1024字节
//Store_Data[0]不要在主函数中赋值！！！Store_Data[0]不要在主函数中赋值！！！Store_Data[0]不要在主函数中赋值！！！
void Store_Init(void)
//这是将FLASH的数据读取出来放到Store_Data数组中；
//如果是第一次使用的话，先在FLASH的最后一页写半字标志位，再写入其余的0，原因可能是为了和SRAM同步？
{
	if(MyFLASH_ReadHalfWord(STORE_START_ADDRESS) != 0XA5A5)//0XA5A5是自定义标志位，用来判断是否为第一次使用
	{
		MyFLASH_ErasePage(STORE_START_ADDRESS);
		MyFLASH_ProgramHalfWord(STORE_START_ADDRESS,0XA5A5);
		for(u16 i = 1;i<STORE_COUNT;i++)//注意此处i要从1开始，因为0时为标志位存储区域，从1开始的才是有效数据
		{
			MyFLASH_ProgramHalfWord(STORE_START_ADDRESS + i*2,0X0000);//因为每个半字要占用2个地址，所以i*2
			
		}
		//经过上面初始化，闪存最后一页的第一个半字是标志位A5A5，剩下的数据全是0
	}
		for(u16 i = 0;i<STORE_COUNT;i++)
		{
			Store_Data[i] = MyFLASH_ReadHalfWord(STORE_START_ADDRESS + i*2);//把0X0800FC00这一页的数据取出来放到Store_Data数组中
		}
}

void Store_Save(void)//这是将Store_Data数组中的数据保存到FLASH中，实现掉电不丢失
{
	MyFLASH_ErasePage(STORE_START_ADDRESS);
	for(u16 i = 0;i<STORE_COUNT;i++)
	{
		MyFLASH_ProgramHalfWord(STORE_START_ADDRESS + i*2,Store_Data[i]);//把0X0800FC00这一页的数据取出来放到Store_Data数组中
	}
}

void Store_Clear(void)
{
	for(u16 i = 1;i<STORE_COUNT;i++)
	{
		Store_Data[i] = 0X0000;//数据清零
	}
	Store_Save();//更新到闪存里
}
