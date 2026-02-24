#include "oled_menu.h"
#include "menu_data.h"
#include "bsp_iwdg.h"
#include "task_sched.h"
#include "System_Init.h"
#include "onenet_handler.h"
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


/**
 * @brief 主函数入口
 *
 * 初始化硬件及系统资源，启动任务调度循环。
 */
int main(void) {
    Delay_Init();                          // 初始化延时函数
    Initialize_Hardware();                 // 硬件初始化
    ReadStoreErrorTime();                  // 读取存储的错误时间
    Check_Reset_Way();                     // 检查复位方式（如看门狗复位则更新计数）
    Initialize_System();                   // 系统初始化
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4); // 设置NVIC优先级分组（全局唯一）

    OLED_UI_Init(&MainMenuPage);           // 初始化OLED UI界面
    MyRTC_Init();                          // 初始化实时时钟
    MYIWD_Init(2000);                      // 初始化独立看门狗（2秒喂狗间隔）

    while (1) {
        DMATaskHandler();                  // 处理DMA相关任务（如扬尘数据采集）
        Handle_Alarm();                    // 处理报警逻辑
        Wrap_OLED_UI();                    // 更新OLED显示内容
        TaskHandler();                     // 执行定时任务（数据发送、时间获取等）
        Handle_Thresholds();               // 处理阈值保存与恢复
        Handle_Network_Data();             // 接收并处理OneNet网络数据
        IWDG_ReloadCounter();              // 喂狗，防止看门狗复位
    }
}

/* ======================== 中断服务函数 ======================== */

/**
 * @brief TIM4中断服务函数
 *
 * 用于处理OLED UI刷新相关的定时中断。
 */
void TIM4_IRQHandler(void) {
    if (TIM_GetITStatus(TIM4, TIM_IT_Update) == SET) {
        OLED_UI_InterruptHandler();        // 调用OLED UI中断处理函数
        TIM_ClearITPendingBit(TIM4, TIM_IT_Update); // 清除中断标志位
    }
}

/**
 * @brief TIM2中断服务函数
 *
 * 用于驱动任务调度器的时间更新。
 */
void TIM2_IRQHandler(void) {
    if (TIM_GetITStatus(TIM2, TIM_IT_Update) == SET) {
        TaskSchedule();                    // 更新任务调度器的时间计数
        TIM_ClearITPendingBit(TIM2, TIM_IT_Update); // 清除中断标志位
    }
}

