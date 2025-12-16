#include "OLED_UI.h"
#include "OLED_UI_MenuData.h"
#include "led.h"
#include "usart.h"
#include "esp8266.h"
#include "onenet.h"
#include "Timer.h"
#include "DHT11.h"
#include "Alarm.h"
#include "MYIWD.h"
#define ESP8266_ONENET_INFO		"AT+CIPSTART=\"TCP\",\"mqtts.heclouds.com\",1883\r\n"
#define TASK_NUM_MAX 		(sizeof(TaskComps) / sizeof(TaskComps[0])) //计算数组大小 
#define DMA_TASK_NUM_MAX 	(sizeof(DMATaskComps) / sizeof(DMATaskComps[0])) //计算数组大小 	
#define DATA_SEND_INTERVAL_MS  	3000  // 数据发送周期3秒
#define DHT11_READ_INTERVAL_MS 	1000  // 传感器读取周期1秒	
#define Led_Flash_INTERVAL_MS 	500  // Led警报灯闪烁周期0.5秒
#define Buzzer_Buzz_INTERVAL_MS 		500  // 蜂鸣器警报“闪烁”周期0.5秒
#define Get_Noise_Data_INTERVAL_MS 		500  // 获取噪声大小数据周期0.5秒
#define Read_Time_INTERVAL_MS 		500  // 时间刷新周期0.5秒
#define Dust_MAX_WAIT_MS 		6000  // 时间刷新周期0.5秒
#define Noise_MAX_WAIT_MS 		6000  // 时间刷新周期0.5秒
#define ALARM_WRITE_INTERVAL_MS 		10000  // 报警记录周期60秒，此值居然能影响噪音数据接收，为何？

/***********关于数据发送指示圆圆心横坐标x的宏***********/
#define DATA_SEND_INDICATOR_X 115	
/***********关于数据发送指示圆圆心纵坐标y的宏***********/
#define DATA_SEND_INDICATOR_Y 5
/***********关于数据发送指示圆半径的宏***********/
#define DATA_SEND_INDICATOR_RADIUS 5
extern u16 Dust_Limit;
extern u16 Noise_Limit;
extern u16 PM2_5_ENV;
//extern u16 PM25_Value;
//extern u16 Noise_Value;
extern u16 decibels;
extern u16 temp;
extern u16 humi;
extern u16 Reset_Count;
bool Noise_Alarm = false;
bool Dust_Alarm = false;
bool Warning_Show_Flag1 = true;
bool Warning_Show_Flag2 = true;
bool Warning_Show_Flag3 = true;
bool Noise_Data_Error_Flag = false;
#if 0
需要注释的内容
#endif
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
/*
现在OLED问题：当某一行因为字符串过长需要动态显示的时候，这一行能够正常从右到左正常显示，
但是如果下一行的字符串也比较长（接近屏幕右侧显示极限）的话，此时正常情况下这一行是不会动态显示的，
因为并没有超过显示极限，但是现在的情况是这本应该是静态的一行似乎被动态的那一行影响了，比如现在屏幕一共有4行，
动态的那行在第二行，静态但接近显示极限那行为第三行，此时按下向下按键，动态行变为第一行，静态行变为第二行，
但静态行似乎受到动态行的影响，字符串整体向左偏移一小段距离。原因未知。250324
初步判断为字符串滚动判断逻辑在边界处理时有所漏洞，指示滚动的变量在没有及时清零；
解决方法：
			1、找到滚动判断代码所在，修改逻辑。
			2、列表中的文字内容短一点，使其不接近平面右侧，或者文字长一点，使其本来也判定为滚动，就是不要在边界状态。
			
除此之外列表的反显条（或者说选中指示条）也有点问题，当没有数据参数只有文本信息时，反显条能够正常将文本信息刚好包含
但是如果有参数的话，会有一个数字裸露在外，比如temp:85，反显条只会包含到8,5就裸露在外了，可以通过多加一个空格解决这个问题，
但这毕竟是治标不治本，而且多加一个空格的话会使得字符串更加接近屏幕右侧，此时有可能会出现上面提到的边界问题。

下划线光标有问题

上面问题已经全部解决：
1、静态行异常偏移问题：1)修改OLED_UI.c中的 PrintMenuElements 函数中的
int16_t StringLength = CalcStringWidth(ChineseFont,ASCIIFont,page->General_MenuItems[i].General_item_text);
修改为：int16_t StringLength = CalcStringWidth(ChineseFont,ASCIIFont,page->General_MenuItems[i].General_item_text,\
*(page->General_MenuItems[i].General_Value));
错误分析：
我的分析：根本原因是字符串宽度的错误计算导致的文字滚动逻辑出错
		若只是：“噪音阈值：%d dBA”，即使不计算动态值，此行也不会偏移，因为即使计算了动态值也没有到达屏幕右侧末端，所以不会偏移
		若为：“噪音阈值：%d dBA ”，若不计算动态值，此行会在某种情况下（下文有介绍）小段偏移后停下，逻辑存在矛盾，
		根据计算（忽略动态值的错误计算）此行不应该偏移，但实际上是应该偏移的，我感觉可能是在按键按下的时候
		在程序的某处进行了正确的逻辑判断，即应该偏移，但按键放开后，又回归到不偏移的错误逻辑，而更让人困扰的是，
		并非是每按下一次按键，该行就会偏移，它是在从第二行到第三行，从第三行到第二行切换之间才会发生小段向左偏移。
DS的分析：问题的核心在于 动态值未被计算导致的滚动误判 和 偏移量生命周期管理不完整。	
！！！关于这个问题，虽然解决了，但现在我还是不太理解为什么这样就能解决。250325！！！

2、OLED_UI.c中PrintMenuElements函数里的OLED_PrintfMixArea函数（1037行）
从OLED_PrintfMixArea(....,page->General_MenuItems[i].General_item_text);
修改为：OLED_PrintfMixArea(....,page->General_MenuItems[i].General_item_text,*(page->General_MenuItems[i].General_Value));
这个很好理解，之前是忽略了动态值导致字符串宽度计算出错从而导致光标宽度不能完全覆盖字符串；

3、新增下划线光标，在OLED_UI.c的ReverseCoordinate函数中（99行）添加，这个很好理解，就不作解释了。


今天得到了一个教训就是要给ROM留有至少4~6kB的余量，不然MCU可能会无法复位，需要重新烧录，
比如今天的Total ROM Size 为63.87kB，这已经相当接近64kB了，如果这时候按下复位键，程序必然崩溃，导致无法复位
需要重新烧录才能解决问题。250420

惑之未解：
	1、需要将startup_stm32f10x_md.s中的第33行改为Stack_Size      EQU     0x00000800
	hmac_sha1.h中的第9行改为#define MAX_MESSAGE_LENGTH		1024
	原因未知250213
	*/



/**
 * @brief 数据发送函数，负责调用OneNet发送数据，并处理成功/失败情况：
 *        - 成功时通过OLED闪烁指示
 *        - 失败超过阈值则记录错误信息、延时并触发看门狗复位
 */
void Data_Send(void) {
    static u8 Send_Data_Error = 0; // 静态变量用于记录连续发送失败次数

    if (OneNet_SendData() == 0) {
        // 数据发送成功：OLED局部刷新，显示一次闪烁提示
        OLED_DrawCircle(DATA_SEND_INDICATOR_X, DATA_SEND_INDICATOR_Y,
                        DATA_SEND_INDICATOR_RADIUS, OLED_FILLED); // 绘制实心圆作为发送指示
        OLED_UpdateArea(DATA_SEND_INDICATOR_X - DATA_SEND_INDICATOR_RADIUS,
                        DATA_SEND_INDICATOR_Y - DATA_SEND_INDICATOR_RADIUS,
                        DATA_SEND_INDICATOR_RADIUS * 2 + 2,
                        DATA_SEND_INDICATOR_RADIUS * 2 + 2); // 局部刷新OLED显示区域

        ESP8266_Clear(); // 清除ESP8266缓冲区（避免残留数据影响下次发送）

        // 恢复OLED状态：清除指示圆，不影响其他内容显示
        OLED_ClearArea(DATA_SEND_INDICATOR_X - DATA_SEND_INDICATOR_RADIUS,
                       DATA_SEND_INDICATOR_Y - DATA_SEND_INDICATOR_RADIUS,
                       DATA_SEND_INDICATOR_RADIUS * 2 + 2,
                       DATA_SEND_INDICATOR_RADIUS * 2 + 2);

        Send_Data_Error = 0; // 发送成功，清空失败计数器
    } else {
        // 数据发送失败，增加失败计数
        Send_Data_Error++;

        // 如果失败次数 >= 2，则进行错误处理
        if (Send_Data_Error >= 2) {
            // 错误处理流程：
            // 1. 设置错误类型为“环境通信数据传输失败”
            // 2. 获取当前时间
            // 3. 存储错误信息到FLASH中
            ErrorType(ENV_COMM_DATA_TRANSMISSION_FAILURE);
            Delay_s(3); // 延时3秒，即将看门狗复位
        }
    }
}
/**
 * @brief DHT11温湿度传感器读取函数
 *        - 成功读取则清空错误计数
 *        - 连续失败达到阈值则记录环境传感器异常错误
 */
void DHT11_Read(void) {
    static u8 DHT11_Error_Flag = 0; // 静态变量用于记录连续读取失败次数

    if (DHT11_Read_Data(&temp, &humi) == 0) {
        // 读取成功：清空错误计数
        DHT11_Error_Flag = 0;
    } else {
        // 读取失败，增加错误计数
        DHT11_Error_Flag++;

        // 如果连续失败次数 >= 5，记录传感器异常错误
        if (DHT11_Error_Flag >= 5) {
            // 调用错误处理函数，设置错误类型为“环境传感器温湿度异常”
            ErrorType(ENV_SENSOR_TEMP_HUMIDITY_ANOMALY);
        }
    }
}



/**
 * @brief 任务调度结构体定义
 *
 * 用于实现基于时间片轮转的任务调度机制。
 * 每个任务对应一个结构体实例，包含执行标志、时间计数和任务函数指针。
 */
typedef struct {
    bool run;               ///< 调度标志：1 表示任务可调度运行，0 表示挂起状态

    uint16_t TimCount;      ///< 时间片递减计数器，当值为0时触发任务执行

    uint16_t TimeRload;     ///< 时间片重载值，用于定时重置 TimCount

    void (*pTaskFunc)(void); ///< 函数指针，指向要执行的任务处理函数
} TaskComps_t;

/**
 * @brief 粉尘（PM2.5）数据读取函数
 *
 * 从串口接收的数据包中解析出PM2.5环境浓度值，并保存到全局变量中。
 */
void Dust_Data_Read(void) {
    // 解析PMS7003传感器发送的串口数据包，获取PM数据
    PM_Data = PMS_ParseDataPacket(Serial_RxPacket, PMS_PACKET_LEN);

    // 提取大气环境中PM2.5的值并存储到全局变量中
    PM2_5_ENV = PM_Data.pm2_5_env;
}

/**
 * @brief PMS7003传感器错误处理函数
 *一旦此函数被调用，则记录错误类型为：”ENV_SENSOR_DUST_ANOMALY“
 * 并进行延时，即将系统复位
 */
void Dust_Data_Error(void){
	ErrorType(ENV_SENSOR_DUST_ANOMALY);
	Delay_s(3);//加上这句就会来不及喂狗，会看门狗复位，
	//现在准备只输出错误信息，但不复位
}
//此函數每隔 ALARM_WRITE_INTERVAL_MS 毫秒执行一次，如果标志位本来就为真，那么什么也不做
//当实际数据超过阈值时，就会将标志位置为假，有没有可能这种情况，
//那就是即将执行 Turn_Warning_Show_Flag 而这时候恰好数据超过了阈值，那么标志位就会快速置为真，
//似乎无法满足每隔 ALARM_WRITE_INTERVAL_MS 才置真这个要求？
//那就算被快速置真了， 为什么会影响到噪音数据的接收呢？我观察到的是确实有噪音数据来，
//但是似乎没通过CRC校验，这是为何呢？是因为对FLASH频繁操作导致的问题吗？
//那为什么没有影响到其他的数据接收，单独影响到了，噪音的接收？
//现在准备改为，当标志位为 false 的时候，定时器对应的重装值才能递减，现看看
/**
 * @brief 环境报警信息写入函数
 *
 * 根据扬尘和噪音报警标志，调用 WarningType 函数设置相应的环境报警类型。
 * 支持三种报警状态：仅扬尘超标、仅噪音超标、两者同时超标。
 */
void Write_Warning_Meg(void) {
    // 判断当前报警状态并设置对应的报警类型
    if (Dust_Alarm && !Noise_Alarm) {
        // 仅扬尘超标
        WarningType(ENV_ALERT_DUST_OVERLIMIT);
    }
    else if (Noise_Alarm && !Dust_Alarm) {
        // 仅噪音超标
        WarningType(ENV_ALERT_NOISE_OVERLIMIT);
    }
    else if (Dust_Alarm && Noise_Alarm) {
        // 扬尘与噪音同时超标
        WarningType(ENV_ALERT_DUST_NOISE_COMBINED);
    }

    // 注：未处理“无报警”情况，即默认不设置任何报警类型
}

//判断噪音数据正确的方式最佳方案应该是Serial_GetRxFlag() == 1&&crc == calcCrc(见XM7903.c)
/**
 * @brief XM7903传感器错误处理函数
 *一旦此函数被调用，则记录错误类型为：” ENV_SENSOR_NOISE_ANOMALY “
 * 并进行延时，即将系统复位
 */
void NOise_Data_Error(void){//暂时我先注释掉这个函数，因为我要用5v口
	ErrorType(ENV_SENSOR_NOISE_ANOMALY);
	Delay_s(3);
}
		
/**
 * @brief 基于DMA的任务结构体定义
 *
 * 用于管理与DMA传输相关联的任务，包含DMA完成标志和对应的任务处理函数。
 * 适用于需要在DMA传输完成后触发特定操作的场景。
 */
typedef struct {
    volatile bool *DMA_Finished_Flag;  ///< 指向DMA传输完成标志的指针，volatile确保编译器不优化访问

    void (*pTaskFunc)(void);           ///< 函数指针，指向DMA完成后要执行的任务处理函数
} DMATaskComps_t;

/**
 * @brief 定义并初始化DMA任务表
 *
 * 每个任务项包含一个DMA完成标志指针和一个任务函数指针。
 * 当对应DMA传输完成后，主循环中调用的任务处理函数将被执行。
 */
static DMATaskComps_t DMATaskComps[] = {
    {&dma_C15_flag, Dust_Data_Read},  // dma_C15_flag：DMA通道15完成标志；Dust_Data_Read：粉尘数据读取任务
};

/**
 * @brief DMA任务处理函数
 *
 * 主循环中调用此函数，轮询所有DMA任务项：
 * - 如果某项的DMA传输已完成（标志为true）
 * - 则调用其绑定的任务函数，并重置完成标志
 */
void DMATaskHandler(void) {
    for (u8 i = 0; i < DMA_TASK_NUM_MAX; i++) {  // 遍历DMA任务数组
        if (*DMATaskComps[i].DMA_Finished_Flag) {  // 检查DMA是否已完成
            if (DMATaskComps[i].pTaskFunc != NULL) {  // 确保任务函数指针有效
                *DMATaskComps[i].DMA_Finished_Flag = false;  // 清除DMA完成标志
                DMATaskComps[i].pTaskFunc();  // 执行与DMA绑定的任务函数
            }
        }
    }
}

/**
 * @brief 定义系统中所有周期性任务的任务表
 *
 * 每个任务由以下字段构成：
 * - run: 当前是否处于运行状态（1：运行；0：挂起）
 * - TimCount: 时间计数器，递减至0时触发任务执行
 * - TimeRload: 时间重载值，用于设置任务执行的时间间隔（单位：ms）
 * - pTaskFunc: 函数指针，指向要执行的任务处理函数
 *
 * 所有任务在主循环中统一调度运行。
 */
static TaskComps_t TaskComps[] = {
    // 数据发送任务：每 DATA_SEND_INTERVAL_MS ms 执行一次 Data_Send
    {0, DATA_SEND_INTERVAL_MS, DATA_SEND_INTERVAL_MS, Data_Send},

    // DHT11温湿度读取任务：每 DHT11_READ_INTERVAL_MS ms 执行一次 DHT11_Read
    {0, DHT11_READ_INTERVAL_MS, DHT11_READ_INTERVAL_MS, DHT11_Read},

    // LED闪烁任务：每 Led_Flash_INTERVAL_MS ms 执行一次 Led_Turn
    {0, Led_Flash_INTERVAL_MS, Led_Flash_INTERVAL_MS, Led_Turn},

    // 蜂鸣器驱动任务：每 Buzzer_Buzz_INTERVAL_MS ms 执行一次 Buzzer_Turn
    {0, Buzzer_Buzz_INTERVAL_MS, Buzzer_Buzz_INTERVAL_MS, Buzzer_Turn},

    // 噪音传感器数据包发送任务：每 Get_Noise_Data_INTERVAL_MS ms 执行一次 XM7903_SendPacket
    {0, Get_Noise_Data_INTERVAL_MS, Get_Noise_Data_INTERVAL_MS, XM7903_SendPacket},

    // 实时时钟读取任务：每 Read_Time_INTERVAL_MS ms 执行一次 MyRTC_ReadTime
    {0, Read_Time_INTERVAL_MS, Read_Time_INTERVAL_MS, MyRTC_ReadTime},

    // 粉尘数据错误处理任务：等待 Dust_MAX_WAIT_MS ms 后触发 Dust_Data_Error（表示六秒内未收到正确数据）
    {0, Dust_MAX_WAIT_MS, Dust_MAX_WAIT_MS, Dust_Data_Error},

    // 噪音数据错误处理任务：等待 Noise_MAX_WAIT_MS ms 后触发 NOise_Data_Error
    {0, Noise_MAX_WAIT_MS, Noise_MAX_WAIT_MS, NOise_Data_Error},

    // 报警信息写入任务：每 ALARM_WRITE_INTERVAL_MS ms 执行一次 Write_Warning_Meg
    {0, ALARM_WRITE_INTERVAL_MS, ALARM_WRITE_INTERVAL_MS, Write_Warning_Meg}
};



/**
 * @brief 任务调度函数，需在定时器中断中定期调用
 *
 * 每个周期性任务的时间计数器 TimCount 会递减。
 * 当 TimCount 减至 0 时，重新加载 TimeRload 值并设置 run 标志为 1，表示该任务可执行。
 *
 * 特殊处理：
 * - 如果是 Dust_Data_Error / NOise_Data_Error 类型错误任务：
 *   - 若传感器数据已正常接收，则重置计时器，避免错误触发
 * - LED 和蜂鸣器报警任务根据当前报警状态决定是否启用
 */
void TaskSchedule(void) {
    for (u8 i = 0; i < TASK_NUM_MAX; i++) {
        if (TaskComps[i].TimCount) {
            TaskComps[i].TimCount--;  // 时间片递减

            /**
             * 粉尘错误任务特殊处理：
             * 若DMA标志位被置位（表示已成功收到数据），则重置计时器，不触发错误任务
             */
            if (TaskComps[i].pTaskFunc == Dust_Data_Error && dma_C15_flag == true) {
                TaskComps[i].TimCount = TaskComps[i].TimeRload; // 正常情况下重置计时器
            }

            /**
             * 噪音错误任务特殊处理：
             * 若未检测到噪音错误标志，则重置计时器，不触发错误任务
             */
            if (TaskComps[i].pTaskFunc == NOise_Data_Error && Noise_Data_Error_Flag == false) {
                TaskComps[i].TimCount = TaskComps[i].TimeRload; // 正常情况下重置计时器
            }

            /**
             * 判断时间片是否耗尽
             * 若耗尽，则重载时间片并设置 run 标志以允许任务运行
             */
            if (TaskComps[i].TimCount == 0) {
                TaskComps[i].TimCount = TaskComps[i].TimeRload;

                /**
                 * 特定任务根据系统状态动态控制是否调度：
                 * - LED闪烁任务：当有扬尘或噪音报警且未手动关闭报警时才启动
                 * - 蜂鸣器任务：只有当两个报警同时存在且未手动关闭报警时才启动
                 * - 其他任务默认允许运行
                 */
                if (TaskComps[i].pTaskFunc == Led_Turn) {
                    TaskComps[i].run = (Noise_Alarm || Dust_Alarm) && !Alarm_Off_Manual;
                }
                else if (TaskComps[i].pTaskFunc == Buzzer_Turn) {
                    TaskComps[i].run = (Noise_Alarm && Dust_Alarm) && !Alarm_Off_Manual;
                }
                else {
                    TaskComps[i].run = 1; // 默认允许任务运行
                }
            }
        }
    }
}
/**
 * @brief 任务执行函数，需在主循环中定期调用
 *
 * 遍历所有任务项，如果任务的 run 标志为1（表示需要调度执行），
 * 则调用对应的任务函数，并清空 run 标志以防止重复执行。
 */
void TaskHandler(void) {
    for (u8 i = 0; i < TASK_NUM_MAX; i++) {  // 遍历所有任务
        if (TaskComps[i].run) {              // 检查任务是否被调度
            if (TaskComps[i].pTaskFunc != NULL) {  // 确保函数指针有效
                TaskComps[i].run = 0;         // 清除运行标志，防止重复执行
                TaskComps[i].pTaskFunc();     // 执行任务函数
            }
        }
    }
}


/**
 * @brief 阈值处理函数
 *
 * 用于处理用户设置的扬尘和噪音阈值保存操作，
 * 以及恢复默认阈值和相关状态信息。
 */
void Handle_Thresholds(void) {
    // 如果需要保存当前阈值到存储器中
    if (Limit_Save) {
        Store_Data[DUST_LIMIT_Store_IDX] = Dust_Limit;     // 保存扬尘报警阈值
        Store_Data[NOISE_LIMIT_Store_IDX] = Noise_Limit;   // 保存噪音报警阈值
        Store_Save();                                      // 调用存储函数，写入非易失性存储（如Flash）
        Limit_Save = false;                                // 清除保存标志，防止重复保存
    }

    // 如果需要清除数据并恢复默认阈值
    if (Clear_Data) {
        Dust_Limit = Default_Dust_Limit;       // 恢复默认扬尘阈值
        Noise_Limit = Default_Noise_Limit;     // 恢复默认噪音阈值

        ErrorTimeReset();                      // 错误记录时间恢复默认
        WarningTimeReset();                    // 报警记录时间恢复默认

        Reset_Count = 0;                       // 系统复位次数清零

        Store_Clear();                         // 清除存储中的用户配置数据
        Clear_Data = false;                    // 清除清除标志，防止重复执行
    }
}

/**
 * @brief 处理从网络接收到的数据
 *
 * 该函数通过调用 ESP8266_GetIPD 获取数据，并使用 OneNet_RevPro 解析和处理接收到的数据。
 * 如果处理过程中出现错误，函数会增加错误计数，并在连续三次错误后调用 ErrorWarningType 报告错误。
 */
void Handle_Network_Data(void) {
    static u8 Rece_Data_Error = 0; // 静态变量，用于累积错误次数
    unsigned char *dataPtr = NULL;

    // 获取数据
    dataPtr = ESP8266_GetIPD(0);
    if (dataPtr != NULL) {
        // 解析和处理数据
        if (OneNet_RevPro(dataPtr) != 0) {
            Rece_Data_Error++;
            // UsartPrintf(USART_DEBUG, "Error: 数据处理失败, 错误次数: %d\r\n", Rece_Data_Error);
            if (Rece_Data_Error >= 3) {
                // 连续三次错误，报告错误
                // UsartPrintf(USART_DEBUG, "Error: 连续三次数据处理失败，报告错误\r\n");
                ErrorType(ENV_COMM_DATA_RECEPTION_FAILURE);
                Delay_s(3);//延时三秒，即将看门狗复位
            }
        } else {
            // 数据处理成功，重置错误计数器
            Rece_Data_Error = 0;
        }
    }
}

/**
 * @brief  处理报警条件。
 * @param  Noise_Alarm: 噪音报警标志位。
 * @param  Dust_Alarm: 扬尘报警标志位。
 * @retval None
 */
void Handle_Alarm(void) {
	Noise_Alarm = (decibels > Noise_Limit);//判断噪音是否大于阈值	
	Dust_Alarm = (PM_Data.pm2_5_env > Dust_Limit);//判断扬尘是否大于阈值	
    // 如果两个报警标志位都为 false 或手动关闭报警，则关闭声光报警
    if ((!Noise_Alarm && !Dust_Alarm) || Alarm_Off_Manual) {
        Alarm_Off_Auto(Led_And_Buzzer);
    }
    // 如果只有一个报警标志位为 true 且蜂鸣器正在响，则关闭蜂鸣器
    if ((Noise_Alarm && !Dust_Alarm) || (!Noise_Alarm && Dust_Alarm)) {
        if (GPIO_ReadOutputDataBit(GPIOB, Buzzer_IO) == 0) {
            Alarm_Off_Auto(Buzzer);
        }
    }
}

/**
 * @brief  初始化硬件。
 * @retval None
 */
void Initialize_Hardware(void) {
    OLED_Init();//OLED屏初始化，与数据显示有关
    Timer2_Init();//定时2初始化，与任务调度有关
    PMS7003_Init();//扬尘传感器初始化
//    AD_Init();//ADC初始化，与声音数据采集有关，现改为XM7903之后，无再进行数据采集，故将此注释
    Store_Init();//FLASH初始化，便于后续存储阈值与独立看门狗次数
    Alarm_Init();//警报初始化
    Usart1_Init(9600);
    Usart2_Init(115200);
	Usart3_Init(9600);   
	
}

/**
 * @brief  初始化系统。
 * @retval None
 */
void Initialize_System(void) {
    u8 retry_count = 0;
    const u8 MAX_RETRY_COUNT = 5; // 最大重试次数
    
    /* *************************************************************************** *\
    *                           系统启动画面显示
    \* *************************************************************************** */
    // 显示启动Logo图标
    OLED_Clear();
    OLED_ShowImageArea(0, 0, 64, 64, 0, 0, 64, 64, USC_LOGO_64);
    OLED_Update();

    /* *************************************************************************** *\
    *                           DHT11传感器初始化
    \* *************************************************************************** */
    // 检测DHT11传感器存在性
    retry_count = 0;
    while(DHT11_Init()) {
        retry_count++;
        OLED_ShowChinese(66, 0, "未找到", OLED_12X12_FULL);
        OLED_ShowString(66, 16, "DHT11!", OLED_7X12_HALF);
        OLED_ShowChinese(66, 48, "重试：", OLED_12X12_FULL);
        OLED_ShowNum(102, 48, retry_count, 2, OLED_7X12_HALF);
        OLED_UpdateArea(66, 0, 127-65, 63);
        
        if(retry_count >= MAX_RETRY_COUNT) {
            OLED_ClearArea(66, 0, 127-65, 63);
            OLED_ShowString(66, 0, "Skip DHT11", OLED_7X12_HALF);
            OLED_ShowString(66, 16, "Continue...", OLED_7X12_HALF);
            OLED_Update();
            Delay_s(3);
            break; // 跳过DHT11初始化，继续后续流程
        }
        Delay_ms(1000);
    }    

    OLED_ClearArea(66, 0, 127-65, 63);
    
    /* *************************************************************************** *\
    *                           ESP8266 WiFi模块初始化
    \* *************************************************************************** */
    // 初始化ESP8266 WiFi模块
    OLED_ShowString(66, 0, "esp8266", OLED_7X12_HALF);
    OLED_ShowChinese(66, 16, "初始化中", OLED_12X12_FULL);
    OLED_Update();
    
    retry_count = 0;
    while(ESP8266_Init()) {
        retry_count++;
        OLED_ShowChinese(66, 16, "初始化失败", OLED_12X12_FULL);
        OLED_ShowChinese(66, 48, "重试：", OLED_12X12_FULL);
        OLED_ShowNum(102, 48, retry_count, 2, OLED_7X12_HALF);
        OLED_Update();
        
        if(retry_count >= MAX_RETRY_COUNT) {
            OLED_ClearArea(66, 0, 127-65, 63);
            OLED_ShowString(66, 0, "ESP8266", OLED_7X12_HALF);
            OLED_ShowChinese(66, 16, "初始化失败", OLED_12X12_FULL);
            OLED_ShowChinese(66, 32, "请检查连接", OLED_12X12_FULL);
            OLED_ShowChinese(66, 48, "或网络状态", OLED_12X12_FULL);
            OLED_Update();
            Delay_s(3);
            NVIC_SystemReset(); // ESP8266初始化失败，系统复位重试
        }
        Delay_ms(2000);
    }
        
    /* *************************************************************************** *\
    *                           OneNET平台TCP连接
    \* *************************************************************************** */
    // 建立与OneNET平台的TCP连接
    OLED_ClearArea(66, 0, 127-65, 63);
    OLED_ShowChinese(66, 0, "正在接入", OLED_12X12_FULL);
    OLED_ShowString(66, 16, "OneNET", OLED_7X12_HALF);    
    OLED_Update();    
    
    retry_count = 0;
    while (ESP8266_SendCmd(ESP8266_ONENET_INFO, "CONNECT")) {
        retry_count++;
        if(retry_count >= MAX_RETRY_COUNT) {
            OLED_ClearArea(66, 0, 127-65, 63);
            OLED_ShowChinese(66, 0, "连接失败", OLED_12X12_FULL);
            OLED_ShowChinese(66, 16, "请检查网络", OLED_12X12_FULL);
            OLED_ShowChinese(66, 32, "即将重试", OLED_12X12_FULL);
            OLED_Update();
            Delay_s(3);
            NVIC_SystemReset(); // TCP连接失败，系统复位重试
        }
        Delay_ms(2000);
    }

    /* *************************************************************************** *\
    *                           OneNET平台MQTT连接
    \* *************************************************************************** */
    // 建立与OneNET平台的MQTT连接
    retry_count = 0;
    while (retry_count < MAX_RETRY_COUNT) {
        unsigned char errorCode = OneNet_DevLink();

        if (errorCode == 0) {
            // 连接成功，跳出重试循环
            break;
        }

        // 显示当前重试信息
        retry_count++;
        OLED_ClearArea(66, 0, 127 - 65, 63);
        OLED_ShowChinese(66, 0, "设备连接", OLED_12X12_FULL);
        OLED_ShowChinese(66, 16, "失败：", OLED_12X12_FULL);
        OLED_ShowNum(104, 16, errorCode, 2, OLED_7X12_HALF);
        OLED_ShowChinese(66, 32, "重试次数:", OLED_12X12_FULL);
        OLED_ShowNum(66, 48, retry_count, 2, OLED_7X12_HALF);
        OLED_Update();

        // 判断是否为不可恢复的错误（无需重试）
        if (errorCode == 1 || errorCode == 2 || errorCode == 4 || errorCode == 5 || 
            errorCode == 6 || errorCode == 7 || errorCode == 9 || errorCode == 10) {
            // 不可恢复错误：鉴权失败、MQTT包构造失败、非CONNACK包、数据包格式错误、
            // 协议版本不可接受、客户端标识符被拒绝、用户名或密码错误、未授权连接
            OLED_ClearArea(66, 0, 127 - 65, 63);
            OLED_ShowChinese(66, 0, "配置错误", OLED_12X12_FULL);
            OLED_ShowChinese(66, 16, "请检查设备", OLED_12X12_FULL);
            OLED_ShowChinese(66, 32, "参数!", OLED_12X12_FULL);
            OLED_Update();
            Delay_s(3);
            NVIC_SystemReset(); // 配置错误，系统复位
        }

        // 可恢复错误（超时或通信失败），继续重试
        if (retry_count >= MAX_RETRY_COUNT) {
            // 重试耗尽
            OLED_ClearArea(66, 0, 127 - 65, 63);
            OLED_ShowChinese(66, 0, "重试耗尽", OLED_12X12_FULL);
            OLED_ShowChinese(66, 16, "请检查网络", OLED_12X12_FULL);
            OLED_ShowChinese(66, 32, "即将重启", OLED_12X12_FULL);
            OLED_Update();
            Delay_s(3);
            NVIC_SystemReset(); // 重试次数用尽，系统复位
        }

        Delay_ms(3000); // 重试间隔
    }
    
    /* *************************************************************************** *\
    *                           OneNET主题订阅及初始化完成提示
    \* *************************************************************************** */
    // 订阅OneNET平台主题
    OneNET_Subscribe();
    
    // 显示初始化完成提示
    OLED_ClearArea(66, 0, 127-65, 63);
    OLED_ShowChinese(66, 0, "连接成功！", OLED_12X12_FULL);
    OLED_ShowChinese(66, 16, "网络初始化", OLED_12X12_FULL);
    OLED_ShowChinese(66, 32, "完毕！即将", OLED_12X12_FULL);
    OLED_ShowChinese(66, 48, "进入系统", OLED_12X12_FULL);
    OLED_Update();    
    Delay_s(1);
    OLED_Clear();    
    
    /* *************************************************************************** *\
    *                           系统参数初始化
    \* *************************************************************************** */
    // 初始化环境监测阈值参数
    Dust_Limit = (Store_Data[DUST_LIMIT_Store_IDX] != 0) ? Store_Data[DUST_LIMIT_Store_IDX] : Default_Dust_Limit;
    Noise_Limit = (Store_Data[NOISE_LIMIT_Store_IDX] != 0) ? Store_Data[NOISE_LIMIT_Store_IDX] : Default_Noise_Limit;
    
    // 读取存储的报警时间信息
    ReadStoreWarningTime(); 
}

/**
 * @brief 获取当前环境噪音值（单位：dBA）
 *
 * 该函数用于从串口接收噪音传感器数据包并解析出噪音值。
 * 若数据异常或未接收到完整数据包，则设置错误标志位 Noise_Data_Error_Flag。
 */
void Get_dBA(void) {
    if (Serial_GetRxFlag() == 1) { // 如果接收到完整数据包
        decibels = (uint16_t)roundf(Get_Nosie_Data()); // 获取原始噪音数据并四舍五入取整

        // 如果返回的噪音值为0，表示数据异常（如CRC校验失败等），设置错误标志
        if (decibels == 0) {
            Noise_Data_Error_Flag = true; // 数据异常标志置为真
        } else {
            Noise_Data_Error_Flag = false; // 数据正常，清除错误标志
        }
    } else {
        // 未接收到有效数据包，设置错误标志
        Noise_Data_Error_Flag = true;
    }
}
int main(){
	Initialize_Hardware();
	ReadStoreErrorTime();
	Check_Reset_Way();//检查复位方式，若是看门狗复位，复位次数加一并储存到FLASH中	    
	Initialize_System();
	OLED_UI_Init(&MainMenuPage);//UI初始化
	MyRTC_Init();//系统时间设置
	MYIWD_Init(2000);//独立看门狗初始化，喂狗间隔为2000ms
	PM_Data.pm2_5_env = 100;//静态警报测试
	decibels = 40;
	PM2_5_ENV = PM_Data.pm2_5_env;
	while(1){
		Get_dBA();//获取噪音数据
		DMATaskHandler();//获取扬尘数据
        Handle_Alarm();// 处理报警条件
		OLED_UI_MainLoop();	//显示刷新
		TaskHandler();//任务处理（包含数据发送、时间获取、警报处理等事件）
		Handle_Thresholds();//保存与恢复默认阈值
		Handle_Network_Data();//接收OneNet数据
		IWDG_ReloadCounter();//喂狗
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
