/**
 * @file task_sched.c
 * @brief 基于时间片轮转 + DMA事件驱动的轻量级任务调度系统
 *
 * 本模块实现两类任务：
 * 1. 定时周期任务（由 SysTick 驱动 TaskSchedule 更新）
 * 2. DMA 事件触发任务（由 DMA 中断标志驱动 DMATaskHandler 执行）
 */

/* ======================== 头文件包含 ======================== */
#include "task_sched.h"
#include "bsp_pms7003.h"
#include "pms7003_drv.h"
#include "sensors_data.h"
#include "stdbool.h"
#include "error_warning_log.h"
#include "bsp_alarm.h" 
#include "menu_data.h"
#include "xm7903_drv.h"
#include "dht11_drv.h"
#include "onenet_handler.h"
#include "system_config.h"

/* ======================== 宏定义 ======================== */
// 任务数组大小计算
#define TASK_NUM_MAX        (sizeof(TaskComps) / sizeof(TaskComps[0]))  ///< 定时任务数量
#define DMA_TASK_NUM_MAX    (sizeof(DMATaskComps) / sizeof(DMATaskComps[0])) ///< DMA任务数量

// 定时任务周期（单位：ms）
#define DATA_SEND_INTERVAL_MS           3000  ///< 数据发送间隔
#define DHT11_READ_INTERVAL_MS          1000  ///< DHT11读取间隔
#define LED_FLASH_INTERVAL_MS           500   ///< LED闪烁间隔
#define BUZZER_BUZZ_INTERVAL_MS         500   ///< 蜂鸣器响铃间隔
#define GET_NOISE_DATA_INTERVAL_MS      500   ///< 噪音数据获取间隔
#define READ_TIME_INTERVAL_MS           500   ///< RTC时间读取间隔
#define GET_SENSORS_DATA_INTERVAL_MS    500   ///< 传感器数据汇总间隔

// 传感器数据超时阈值
#define Dust_MAX_WAIT_MS                6000  ///< 粉尘数据最大等待时间
#define Noise_MAX_WAIT_MS               6000  ///< 噪音数据最大等待时间

// 报警与指示相关
#define ALARM_WRITE_INTERVAL_MS         10000 ///< 报警消息写入间隔
#define ONENET_REPLY_INDICATOR_MS       20    ///< OneNet响应指示间隔

// OLED 成功指示圆参数
#define DATA_SEND_INDICATOR_X           115   ///< 指示圆X坐标
#define DATA_SEND_INDICATOR_Y           5     ///< 指示圆Y坐标
#define DATA_SEND_INDICATOR_RADIUS      5     ///< 指示圆半径

/* ======================== 全局变量 ======================== */

bool Noise_Alarm = false;          ///< 噪音报警状态
bool Dust_Alarm  = false;          ///< 粉尘报警状态

/* ======================== 类型定义 ======================== */

/**
 * @brief 任务调度结构体（时间驱动）
 */
typedef struct {
    bool run;                   ///< 是否可执行
    uint16_t TimCount;          ///< 当前计数值
    uint16_t TimeRload;         ///< 重载值（周期）
    void (*pTaskFunc)(void);    ///< 任务函数指针
} TaskComps_t;

/**
 * @brief DMA 事件任务结构体
 */
typedef struct {
    volatile bool *DMA_Finished_Flag;  ///< DMA 完成标志指针
    void (*pTaskFunc)(void);           ///< 任务函数指针
} DMATaskComps_t;

/**
 * @brief 成功指示器状态枚举
 */
typedef enum {
    INDICATOR_OFF,  ///< 指示器关闭
    INDICATOR_ON    ///< 指示器开启
} indicator_state_t;

static indicator_state_t g_success_indicator = INDICATOR_OFF; ///< 成功指示器当前状态
static uint32_t g_success_indicator_start_time = 0;           ///< 指示器启动时间戳

/* ======================== 定时任务函数 ======================== */

/**
 * @brief 读取DHT11温湿度数据
 *
 * 从DHT11传感器读取温度和湿度数据，并更新全局传感器数据。
 * 若连续5次读取失败，则触发错误处理。
 */
void DHT11_Read(void) {
    static u8 DHT11_Error_Flag = 0;
    u16 temp = 0, humi = 0;
    if (DHT11_Read_Data(&temp, &humi)) {
        SensorsData_Update_Temp_Humi(&temp, &humi);
        DHT11_Error_Flag = 0;
    } else {
        if (++DHT11_Error_Flag >= 5) {
            WarningType(ENV_SENSOR_TEMP_HUMIDITY_ANOMALY);//降级为警报
        }
    }
}

/**
 * @brief 处理XM7903噪音传感器任务
 *
 * 发送查询指令并接收XM7903传感器数据，解析后更新全局噪音数据。
 */
void XM7903_Task(void) {
    XM7903_Data_t data;
    BSP_XM7903_SendQuery();
    BSP_XM7903_StartReceive();

    if (xm7903_rx_ready) {
        xm7903_rx_ready = false;
        const uint8_t *buf = BSP_XM7903_GetRxBuffer();
        data = XM7903_Parse(buf);
    } else {
        data.valid = false;
    }
    SensorsData_Update_Noise(&data);
}

/**
 * @brief 粉尘数据超时错误处理
 *
 * 触发粉尘传感器数据超时错误，并复位
 */
void Dust_Data_Error(void) {
    ErrorType(ENV_SENSOR_DUST_ANOMALY);
	NVIC_SystemReset();
}

/**
 * @brief 噪音数据超时错误处理
 *
 * 触发噪音传感器数据超时错误，并复位
 */
void Noise_Data_Error(void) {
    ErrorType(ENV_SENSOR_NOISE_ANOMALY);
	NVIC_SystemReset();
}

/**
 * @brief 写入报警消息
 *
 * 根据当前报警状态写入相应的报警消息。
 */
void Write_Warning_Meg(void) {
    if (Dust_Alarm && !Noise_Alarm) {
        WarningType(ENV_ALERT_DUST_OVERLIMIT);
    } else if (Noise_Alarm && !Dust_Alarm) {
        WarningType(ENV_ALERT_NOISE_OVERLIMIT);
    } else if (Dust_Alarm && Noise_Alarm) {
        WarningType(ENV_ALERT_DUST_NOISE_COMBINED);
    }
}

/**
 * @brief 更新成功指示器状态
 *
 * 控制OLED屏幕上的成功指示圆显示状态。
 */
void Update_Success_Indicator(void) {
    if (g_success_indicator == INDICATOR_ON) {
        if (SysTick_Get() - g_success_indicator_start_time >= 300) {
            g_success_indicator = INDICATOR_OFF;
        }
    }
}

/* ======================== DMA 事件任务函数 ======================== */

/**
 * @brief PMS7003粉尘传感器任务
 *
 * 解析PMS7003接收缓冲区的数据包，并更新全局PM数据。
 */
void PMS7003_Task(void) {
    PM_SensorData data = PMS_ParseDataPacket(BSP_PMS7003_GetRxBuffer());
    if (data.is_valid) {
        SensorsData_Update_PM(&data);
    }
}

/* ======================== 任务表定义 ======================== */

/** @brief 定时任务表 */
static TaskComps_t TaskComps[] = {
	{0, GET_NOISE_DATA_INTERVAL_MS,  GET_NOISE_DATA_INTERVAL_MS,  XM7903_Task},
    {0, DATA_SEND_INTERVAL_MS,       DATA_SEND_INTERVAL_MS,       Data_Send},
    {0, DHT11_READ_INTERVAL_MS,      DHT11_READ_INTERVAL_MS,      DHT11_Read},
    {0, LED_FLASH_INTERVAL_MS,       LED_FLASH_INTERVAL_MS,       Led_Turn},
    {0, BUZZER_BUZZ_INTERVAL_MS,     BUZZER_BUZZ_INTERVAL_MS,     Buzzer_Turn},
    {0, READ_TIME_INTERVAL_MS,       READ_TIME_INTERVAL_MS,       MyRTC_ReadTime},
    {0, Dust_MAX_WAIT_MS,            Dust_MAX_WAIT_MS,            Dust_Data_Error},
    {0, Noise_MAX_WAIT_MS,           Noise_MAX_WAIT_MS,           Noise_Data_Error},
    {0, ALARM_WRITE_INTERVAL_MS,     ALARM_WRITE_INTERVAL_MS,     Write_Warning_Meg},
    {0, ONENET_REPLY_INDICATOR_MS,   ONENET_REPLY_INDICATOR_MS,   Update_Success_Indicator},

};

/** @brief DMA任务表 */
static DMATaskComps_t DMATaskComps[] = {
    {&pms7003_rx_ready, PMS7003_Task}
};

/* ======================== 调度器实现 ======================== */

/**
 * @brief 定时任务调度器
 *
 * 更新定时任务的时间计数器，当计数归零时标记任务为可执行。
 */
void TaskSchedule(void) {
    for (u8 i = 0; i < TASK_NUM_MAX; i++) {
        if (TaskComps[i].TimCount > 0) {
            TaskComps[i].TimCount--;

            // 粉尘数据正常则重置错误计时器
            if (TaskComps[i].pTaskFunc == Dust_Data_Error) {
				const SensorsData_t* data = SensorsData_Get();
				if(data->pm.is_valid) {
					TaskComps[i].TimCount = TaskComps[i].TimeRload;
				}
                
            }

            // 噪音数据有效则重置错误计时器
            if (TaskComps[i].pTaskFunc == Noise_Data_Error) {
                const SensorsData_t* data = SensorsData_Get();
                if (data->noise.valid) {
                    TaskComps[i].TimCount = TaskComps[i].TimeRload;
                }
            }

            // 时间到，准备执行
            if (TaskComps[i].TimCount == 0) {
                TaskComps[i].TimCount = TaskComps[i].TimeRload;

                // 动态控制报警任务
                if (TaskComps[i].pTaskFunc == Led_Turn) {
                    TaskComps[i].run = (Noise_Alarm || Dust_Alarm) && !Alarm_Off_Manual;
                } else if (TaskComps[i].pTaskFunc == Buzzer_Turn) {
                    TaskComps[i].run = (Noise_Alarm && Dust_Alarm) && !Alarm_Off_Manual;
                } else {
                    TaskComps[i].run = 1;
                }
            }
        }
    }
}

/**
 * @brief 定时任务执行器
 *
 * 遍历任务表，执行所有标记为可执行的任务。
 */
void TaskHandler(void) {
    for (u8 i = 0; i < TASK_NUM_MAX; i++) {
        if (TaskComps[i].run && TaskComps[i].pTaskFunc) {
            TaskComps[i].run = 0;
            TaskComps[i].pTaskFunc();
        }
    }
}

/**
 * @brief DMA任务执行器
 *
 * 遍历DMA任务表，执行已完成DMA传输的任务。
 */
void DMATaskHandler(void) {
    for (u8 i = 0; i < DMA_TASK_NUM_MAX; i++) {
        if (*DMATaskComps[i].DMA_Finished_Flag && DMATaskComps[i].pTaskFunc) {
            *DMATaskComps[i].DMA_Finished_Flag = false;
            DMATaskComps[i].pTaskFunc();
        }
    }
}

/* ======================== 系统辅助函数 ======================== */

/**
 * @brief 处理报警逻辑
 *
 * 根据传感器数据判断是否触发报警，并自动关闭部分报警设备。
 */
void Handle_Alarm(void) {
    const SensorsData_t* data = SensorsData_Get();
    bool noise_alarm = (data->noise.noise_db > (float)Noise_Limit);
    bool dust_alarm  = (data->pm.pm2_5_env > Dust_Limit);

    Noise_Alarm = noise_alarm;
    Dust_Alarm  = dust_alarm;

    if ((!noise_alarm && !dust_alarm) || Alarm_Off_Manual) {
        Alarm_Off_Auto(Led_And_Buzzer);
    } else if ((noise_alarm != dust_alarm) &&
               (GPIO_ReadOutputDataBit(GPIOB, Buzzer_IO) == 0)) {
        Alarm_Off_Auto(Buzzer);
    }
}

/**
 * @brief 处理阈值配置
 *
 * 保存或清除传感器报警阈值，并同步到存储器。
 */
void Handle_Thresholds(void) {
    if (Limit_Save) {
        Store_Data[DUST_LIMIT_STORE_IDX] = Dust_Limit;
        Store_Data[NOISE_LIMIT_STORE_IDX] = Noise_Limit;
        Store_Save();
        Limit_Save = false;
    }

    if (Clear_Data) {
        Dust_Limit = Default_Dust_Limit;
        Noise_Limit = Default_Noise_Limit;
        ErrorTimeReset();
        WarningTimeReset();
        Reset_Count = 0;
        Store_Clear();
        Clear_Data = false;
    }
}

/**
 * @brief 触发成功指示器
 *
 * 启动OLED屏幕上的成功指示圆显示。
 */
void Trigger_Success_Indicator(void) {
    if (g_success_indicator == INDICATOR_OFF) {
        g_success_indicator = INDICATOR_ON;
        g_success_indicator_start_time = SysTick_Get();
    }
}

/**
 * @brief 包装OLED UI绘制逻辑
 *
 * 绘制主界面并叠加成功指示圆。
 */
void Wrap_OLED_UI(void) {
    OLED_UI_MainLoop();
    if (g_success_indicator == INDICATOR_ON) {
        OLED_DrawCircle(DATA_SEND_INDICATOR_X, DATA_SEND_INDICATOR_Y,
                        DATA_SEND_INDICATOR_RADIUS, OLED_FILLED);
    }
    OLED_Update();
}
