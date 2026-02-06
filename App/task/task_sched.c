#include "task_sched.h"
#include "bsp_pms7003.h"
#include "sensors_data.h"
#define TASK_NUM_MAX 		(sizeof(TaskComps) / sizeof(TaskComps[0])) //计算数组大小 
#define DMA_TASK_NUM_MAX 	(sizeof(DMATaskComps) / sizeof(DMATaskComps[0])) //计算数组大小 	

#define DATA_SEND_INTERVAL_MS  	3000  // 数据发送周期3秒
#define DHT11_READ_INTERVAL_MS 	1000  // 传感器读取周期1秒	
#define LED_FLASH_INTERVAL_MS 	500  // Led警报灯闪烁周期0.5秒
#define BUZZER_BUZZ_INTERVAL_MS 		500  // 蜂鸣器警报“闪烁”周期0.5秒
#define GET_NOISE_DATA_INTERVAL_MS 		500  // 获取噪声大小数据周期0.5秒
#define READ_TIME_INTERVAL_MS 		500  // 时间刷新周期0.5秒
#define Dust_MAX_WAIT_MS 		6000  // 最长合法数据等待时间（如果超过这个时间还没有合法数据来，就会触发对应函数）
#define Noise_MAX_WAIT_MS 		6000  // 最长合法数据等待时间
#define ALARM_WRITE_INTERVAL_MS 		10000  // 报警记录周期60秒，此值居然能影响噪音数据接收，为何？
#define ONENET_REPLY_INDICATOR_MS 		20 
/***********关于数据发送指示圆圆心横坐标x的宏***********/
#define DATA_SEND_INDICATOR_X 115	
/***********关于数据发送指示圆圆心纵坐标y的宏***********/
#define DATA_SEND_INDICATOR_Y 5
/***********关于数据发送指示圆半径的宏***********/
#define DATA_SEND_INDICATOR_RADIUS 5

extern u16 temp;
extern u16 humi;
extern u16 decibels;
extern u16 Dust_Limit;
extern u16 Noise_Limit;
extern u16 Reset_Count;
bool Noise_Alarm = false;
bool Dust_Alarm = false;

/*------------------------这些变量与OLED显示屏上面的闪烁指示圆有关----------------------*/
typedef enum {
    INDICATOR_OFF,
    INDICATOR_ON
} indicator_state_t;

static indicator_state_t g_success_indicator = INDICATOR_OFF;
static uint32_t g_success_indicator_start_time = 0;

//相关函数 Trigger_Success_Indicator 、Update_Success_Indicator、Wrap_OLED_UI

/*===========================================================*/


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



/*---------------------------定时器控制任务------------------------------*/
//需要用定时器控制定时执行的函数都放这儿！必须是无返回值无传参！

/**
 * @brief DHT11温湿度传感器读取函数
 *        - 成功读取则清空错误计数
 *        - 连续失败达到阈值则记录环境传感器异常错误
 */
void DHT11_Read(void) {
    static u8 DHT11_Error_Flag = 0; // 静态变量用于记录连续读取失败次数
    u16 temp = 0;
	u16 humi = 0;
    if (DHT11_Read_Data(&temp, &humi)) {
        // 读取成功：清空错误计数
		SensorsData_Update_Temp_Humi(&temp, &humi);
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
 * @brief XM7903传感器数据采集任务
 * 
 * 执行一次完整的传感器数据读取流程：
 * 1. 发送MODBUS查询命令（阻塞式，等待发送完成）
 * 2. 启动DMA接收以等待传感器响应
 * 3. 检查DMA接收是否已完成，若完成则解析响应数据
 * 
 * 该函数应在合适的时间间隔内定期调用（如每1～2秒），以实现连续的数据采集。
 * 
 * @note 该函数整体是非阻塞的（不主动延时等待响应），
 *       但由于发送采用阻塞轮询，实际耗时取决于串口波特率和帧长。
 *       响应是否能在本次调用中处理，取决于传感器响应速度与DMA传输完成时机。
 *       若本次未收到响应，则标记数据为无效。
 * 
 * @see BSP_XM7903_SendQuery      // 阻塞式发送
 * @see BSP_XM7903_StartReceive   // 启动DMA接收（非阻塞）
 * @see BSP_XM7903_GetRxBuffer
 * @see XM7903_Parse
 */
void XM7903_Task(void)
{
    XM7903_Data_t data;
    BSP_XM7903_SendQuery();      // 阻塞发送
    BSP_XM7903_StartReceive();   // 启动DMA接收

    if (xm7903_rx_ready) {
        xm7903_rx_ready = false;
        const uint8_t *buf = BSP_XM7903_GetRxBuffer();
        data = XM7903_Parse(buf); // 内部已处理 CRC 失败情况
        SensorsData_Update_Noise(&data);
    } else {
        // 未收到响应：明确标记为无效
        data.valid = false;
        SensorsData_Update_Noise(&data);
    }
}

/**
 * @brief PMS7003传感器错误处理函数
 *一旦此函数被调用，则记录错误类型为：”ENV_SENSOR_DUST_ANOMALY“
 * 并进行延时，即将系统复位
 */
void Dust_Data_Error(void){
	ErrorType(ENV_SENSOR_DUST_ANOMALY);
//	NVIC_SystemReset();
	//现在准备只输出错误信息，但不复位
}

/**
 * @brief XM7903传感器错误处理函数
 *一旦此函数被调用，则记录错误类型为：” ENV_SENSOR_NOISE_ANOMALY “
 * 并进行延时，即将系统复位
 */
void Noise_Data_Error(void){
	ErrorType(ENV_SENSOR_NOISE_ANOMALY);
//	NVIC_SystemReset();//直接复位不依赖看门狗了
}

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

void Update_Success_Indicator(void)
{
    if (g_success_indicator == INDICATOR_ON)
    {
        if (SysTick_Get() - g_success_indicator_start_time >= 300) // 亮 0.3 秒
        {
            g_success_indicator = INDICATOR_OFF;
        }
    }
}

/*=======================定时器控制任务====================================*/


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
    {0, LED_FLASH_INTERVAL_MS, LED_FLASH_INTERVAL_MS, Led_Turn},

    // 蜂鸣器驱动任务：每 Buzzer_Buzz_INTERVAL_MS ms 执行一次 Buzzer_Turn
    {0, BUZZER_BUZZ_INTERVAL_MS, BUZZER_BUZZ_INTERVAL_MS, Buzzer_Turn},

    // 噪音传感器数据包发送任务：每 Get_Noise_Data_INTERVAL_MS ms 执行一次 XM7903_Task
//    {0, GET_NOISE_DATA_INTERVAL_MS, GET_NOISE_DATA_INTERVAL_MS, XM7903_Task},

    // 实时时钟读取任务：每 Read_Time_INTERVAL_MS ms 执行一次 MyRTC_ReadTime
    {0, READ_TIME_INTERVAL_MS, READ_TIME_INTERVAL_MS, MyRTC_ReadTime},

    // 粉尘数据错误处理任务：等待 Dust_MAX_WAIT_MS ms 后触发 Dust_Data_Error（表示六秒内未收到正确数据）
    {0, Dust_MAX_WAIT_MS, Dust_MAX_WAIT_MS, Dust_Data_Error},

    // 噪音数据错误处理任务：等待 Noise_MAX_WAIT_MS ms 后触发 NOise_Data_Error
    {0, Noise_MAX_WAIT_MS, Noise_MAX_WAIT_MS, Noise_Data_Error},

    // 报警信息写入任务：每 ALARM_WRITE_INTERVAL_MS ms 执行一次 Write_Warning_Meg
    {0, ALARM_WRITE_INTERVAL_MS, ALARM_WRITE_INTERVAL_MS, Write_Warning_Meg},
	
	// OneNET响应指示状态刷新任务：每 ALARM_WRITE_INTERVAL_MS ms 执行一次 Write_Warning_Meg
    {0, ONENET_REPLY_INDICATOR_MS, ONENET_REPLY_INDICATOR_MS, Update_Success_Indicator}
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
            if (TaskComps[i].pTaskFunc == Dust_Data_Error && pms7003_rx_ready) {
                TaskComps[i].TimCount = TaskComps[i].TimeRload; // 正常情况下重置计时器
            }

            /**
             * 噪音错误任务特殊处理：
             * 若未检测到噪音错误标志，则重置计时器，不触发错误任务
             */
//            if (TaskComps[i].pTaskFunc == Noise_Data_Error && g_xm7903_data.valid) {
//                TaskComps[i].TimCount = TaskComps[i].TimeRload; // 正常情况下重置计时器
//            }

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
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/








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


/*--------------------------------DMA事件驱动函数-------------------------------------*/
//由DMA完成标志位控制执行的函数放这儿！无返回值，无传参！

/**
 * @brief 粉尘（PM2.5）数据读取函数
 *
 * 从串口接收的数据包中解析出PM2.5环境浓度值，并推送到sensors_data里面统一管理
 */
void PMS7003_Task(void) {
    // 从 DMA 接收缓冲区解析数据
    PM_SensorData data = PMS_ParseDataPacket(BSP_PMS7003_GetRxBuffer());
    
    if (data.is_valid) {
        // ✅ 将解析结果“推送”到中心存储
        SensorsData_Update_PM(&data);
        
    }
	
}

/*===============================DMA事件驱动函数==================================*/

/**
 * @brief 定义并初始化DMA任务表
 *
 * 每个任务项包含一个DMA完成标志指针和一个任务函数指针。
 * 当对应DMA传输完成后，主循环中调用的任务处理函数将被执行。
 */
static DMATaskComps_t DMATaskComps[] = {
    {&pms7003_rx_ready, PMS7003_Task}  // pms7003_rx_ready：DMA通道15完成标志；Dust_Data_Read：粉尘数据读取任务
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

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/



/*---------------------------------系统辅助函数----------------------------------------------*/
//这里存放一些辅助函数，他们通常是完成系统的某个部分逻辑处理，但是还没有必要封装成独立文件的函数
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
 * @brief 阈值处理函数
 *
 * 用于处理用户设置的扬尘和噪音阈值保存操作，
 * 以及恢复默认阈值和相关状态信息。
 */
void Handle_Thresholds(void) {
    // 如果需要保存当前阈值到存储器中
    if (Limit_Save) {
        Store_Data[DUST_LIMIT_STORE_IDX] = Dust_Limit;     // 保存扬尘报警阈值
        Store_Data[NOISE_LIMIT_STORE_IDX] = Noise_Limit;   // 保存噪音报警阈值
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




void Trigger_Success_Indicator(void)
{
    if (g_success_indicator == INDICATOR_OFF)
    {
        g_success_indicator = INDICATOR_ON;
        g_success_indicator_start_time = SysTick_Get();
    }
}

//这个函数要用来包裹原来的UI，在原显示至上再覆盖一个指示闪烁圆，用来指示数据已被平台响应，
//指示圆亮灭及其显示时长由 Trigger_Success_Indicator 与 Update_Success_Indicator共同决定
void Wrap_OLED_UI(void) {
	
	
	OLED_UI_MainLoop();	//显示刷新
	// ✅ 新增：在所有 UI 绘制完成后，叠加成功指示器
    if (g_success_indicator == INDICATOR_ON)
    {
        OLED_DrawCircle(DATA_SEND_INDICATOR_X, DATA_SEND_INDICATOR_Y,
                        DATA_SEND_INDICATOR_RADIUS, OLED_FILLED);
    }
	OLED_Update();
}

/*=====================================任务暂存地======================================*/





         
     














