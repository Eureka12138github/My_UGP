#include "oled_menu.h"
#include "menu_data.h"
#include "bsp_usart.h"
#include "pms7003_drv.h"
#include "xm7903_drv.h"
#include "esp8266_drv.h"
#include "onenet_mqtt.h"
#include "bsp_timer.h"
#include "dht11_drv.h"
#include "bsp_alarm.h"
#include "bsp_iwdg.h"
#include "task_sched.h"
#include "System_Init.h"
/*
重要：编译器的优化等级要开高点才能编译通过，不然FLASH就满了!
方法：点击魔术棒，点开C/C++页面，将"Optimization"等级改为Level 3(-o3)
编译器优化等级低时：适用于代码调试，在开发时使用；
编译器优化等级高时：适用于最终版本，在开发即将完成时使用；
*/
// 接线说明：
//
// ================== PMS7003 ==================
// PIN1 -> 5V
// PIN2 -> 5V
// PIN3 -> GND
// PIN4 -> GND
// PIN5 -> PB5
// PIN6 -> 无需连接
// PIN7 -> PA9 (USART1_TX)
// PIN8 -> 无需连接
// PIN9 -> PA10 (USART1_RX)
// PIN10 -> PB6
//
// ================== XM7903 ==================
// 5V   -> 5V
// GND  -> GND
// RXD  -> PB10 (USART3_TX)
// TXD  -> PB11 (USART3_RX)
// AD   -> 无需连接
//
// ================== 按键 ====================
// 按键1 -> PA6
// 按键2 -> PA7
// 按键3 -> PB0
// 按键4 -> PB1
//
// ================== 报警模块 ================
// LED    -> PC13
// Buzzer -> PB7
//
// ================== DHT11 ===================
// DATA -> PB12
//
// ================== ESP8266-01S =============
// RST -> PA4
// RX  -> PA2 (USART2_TX)
// TX  -> PA3 (USART2_RX)
// VCC -> 3.3V
// GND -> GND
//
// ================== OLED ====================
// GND -> GND
// VCC -> 3.3V
// SCL -> PB8
// SDA -> PB9




int main(){
	Delay_Init();
	Initialize_Hardware();
	ReadStoreErrorTime();
	Check_Reset_Way();//检查复位方式，若是看门狗复位，复位次数加一并储存到FLASH中	    
	Initialize_System();
	//此分组配置在整个工程中仅需调用一次
    //若有多个中断，可以把此代码放在main函数内，while循环之前
	//若调用多次配置分组的代码，则后执行的配置会覆盖先执行的配置
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2); // 全局唯一调用点
	
	OLED_UI_Init(&MainMenuPage);//UI初始化
	MyRTC_Init();//系统时间设置
//	MYIWD_Init(2000);//独立看门狗初始化，喂狗间隔为2000ms
	PM_Data.pm2_5_env = 100;//静态警报测试
	decibels = 67;	
	
	while(1){
		decibels = g_xm7903_data.noise_db;
		DMATaskHandler();//获取扬尘数据
        Handle_Alarm();// 处理报警条件
		Wrap_OLED_UI();
		TaskHandler();//任务处理（包含数据发送、时间获取、警报处理等事件）
		Handle_Thresholds();//保存与恢复默认阈值
		Handle_Network_Data();//接收OneNet数据
//		IWDG_ReloadCounter();//喂狗
	}
}

//中断函数
void TIM4_IRQHandler(void)
{
	if (TIM_GetITStatus(TIM4, TIM_IT_Update) == SET)
	{
		OLED_UI_InterruptHandler();
		
		TIM_ClearITPendingBit(TIM4, TIM_IT_Update);
	}
}

void TIM2_IRQHandler(void)
{
    // 检查是否有更新中断发生
    if (TIM_GetITStatus(TIM2, TIM_IT_Update) == SET)
    {
		TaskSchedule();
        // 清除更新中断的标志位
        TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
    }

}

