#include "MyRTC.h"
//MYRTC Time = {
//	.Year = 2025,
//	.Month = 4,
//	.Day = 24,
//	.Hour = 12,
//	.Minute = 04,
//	.Second = 30,
//	.wday = 4
//};

MYRTC Time = {
	.Year = 2025,
	.Month = 12,
	.Day = 14,
	.Hour = 11,
	.Minute = 40,
	.Second = 00,
	.wday = 0
};

//这些变量主要用于记录错误发生的时间
ErrorTimeStructure ErrorTime[] = {
{{.Year = 1970,.Month = 1,.Day = 1,.Hour = 0,.Minute = 0,.Second = 0,.wday = 4},0,false},
{{.Year = 1970,.Month = 1,.Day = 2,.Hour = 0,.Minute = 0,.Second = 0,.wday = 4},0,false},
{{.Year = 1970,.Month = 1,.Day = 3,.Hour = 0,.Minute = 0,.Second = 0,.wday = 4},0,false},
};

WarningTimeStructure WarningTime[] = {
{{.Year = 1970,.Month = 1,.Day = 1,.Hour = 0,.Minute = 0,.Second = 0,.wday = 4},0,false},
{{.Year = 1970,.Month = 1,.Day = 2,.Hour = 0,.Minute = 0,.Second = 0,.wday = 4},0,false},
{{.Year = 1970,.Month = 1,.Day = 3,.Hour = 0,.Minute = 0,.Second = 0,.wday = 4},0,false},
};

void ErrorTimeReset(void){
	for(u8 i = 0;i<(sizeof(ErrorTime)/sizeof(ErrorTime[0]));i++){
		ErrorTime[i].errortime.Year = 1970;
		ErrorTime[i].errortime.Month = 1;
		ErrorTime[i].errortime.Day = 1;
		ErrorTime[i].errortime.Hour = 0;
		ErrorTime[i].errortime.Minute = 0;
		ErrorTime[i].errortime.Second = 0;
		ErrorTime[i].errortime.wday = 4;
		ErrorTime[i].errortype = 0;
		ErrorTime[i].errorshowflag = false;
	}
}
void WarningTimeReset(void){
	for(u8 i = 0;i<(sizeof(ErrorTime)/sizeof(ErrorTime[0]));i++){
		WarningTime[i].warningtime.Year = 1970;
		WarningTime[i].warningtime.Month = 1;
		WarningTime[i].warningtime.Day = 1;
		WarningTime[i].warningtime.Hour = 0;
		WarningTime[i].warningtime.Minute = 0;
		WarningTime[i].warningtime.Second = 0;
		WarningTime[i].warningtime.wday = 4;
		WarningTime[i].warningtype = 0;
		WarningTime[i].warningshowflag = false;
	}
}
void MyRTC_SetTime(void);

/**
 * @brief  初始化实时时钟(RTC)并配置相关硬件设置
 * 
 * 本函数完成以下功能：
 * 1. 启用PWR和BKP外设时钟以访问备份域
 * 2. 配置LSE(低速外部晶振)作为RTC时钟源
 * 3. 通过备份寄存器判断首次初始化状态，避免系统复位导致时间重置
 * 4. 完成RTC时钟配置、预分频器设置和时间基准初始化
 * 
 * @note 设计特点：
 * - 使用BKP_DR1寄存器存储初始化标志(0xA5A5)
 * - 首次初始化时配置LSE并设置RTC时间基准
 * - 后续复位时保留RTC配置，需VBAT备用电源维持备份域供电
 * - 所有RTC寄存器操作需等待前次操作完成(RTC_WaitForLastTask)
 * 
 * @warning 硬件依赖：
 * - 需要连接32.768kHz低速晶振
 * - 依赖STM32标准外设库的RCC/PWR/BKP/RTC驱动
 * 
 * 工作流程：
 * 1. 使能PWR和BKP时钟，获取备份域写权限
 * 2. 检查BKP_DR1初始化标志：
 *    - 未初始化(标志不匹配)：执行完整初始化流程
 *      a. 启动LSE晶振并等待就绪
 *      b. 选择LSE作为RTC时钟源并启用
 *      c. 配置预分频器(32768-1)生成1Hz时钟
 *      d. 调用MyRTC_SetTime设置初始时间
 *      e. 写入初始化标志到备份寄存器
 *    - 已初始化(标志匹配)：仅使能RTC时钟并同步
 */
void MyRTC_Init(void)
{
    /* 启用电源接口和备份域时钟 */
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_BKP, ENABLE);

    /* 允许访问备份寄存器 */
    PWR_BackupAccessCmd(ENABLE);

    /* 检查是否首次初始化 */
    if (BKP_ReadBackupRegister(BKP_DR1) != 0XA5A5) 
    {
        /* 完整初始化流程 */
        RCC_LSEConfig(RCC_LSE_ON);  // 启动LSE晶振
        while (RCC_GetFlagStatus(RCC_FLAG_LSERDY) == RESET); // 等待晶振稳定

        RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE); // 选择LSE作为RTC时钟源
        RCC_RTCCLKCmd(ENABLE);        // 使能RTC时钟
        
        /* 等待RTC寄存器同步 */
        RTC_WaitForSynchro();
        RTC_WaitForLastTask();        // 确保寄存器可写
        
        /* 配置预分频器生成1Hz时钟：(32768 / (32768-1 + 1)) */
        RTC_SetPrescaler(32768 - 1);  
        RTC_WaitForLastTask();
        
        MyRTC_SetTime();              // 设置初始时间
        
        BKP_WriteBackupRegister(BKP_DR1, 0XA5A5); // 写入初始化标志
    }
    else 
    {
        /* 已初始化时的处理 */
        RCC_RTCCLKCmd(ENABLE);   // 重新使能RTC时钟
        RTC_WaitForSynchro();    // 同步寄存器
    }
}

/**
 * @brief  设置RTC计数器为指定日期时间的Unix时间戳
 * 
 * @note 本函数完成以下功能：
 * 1. 将用户自定义时间结构体转换为标准tm结构
 * 2. 进行时区修正（北京时间转UTC时间）
 * 3. 生成Unix时间戳并写入RTC计数器
 * 
 * @warning 硬件依赖：
 * - 依赖全局时间结构体Time存储原始时间参数
 * - 假设Time结构体包含Year/Month/Day/Hour/Minute/Second成员
 * - 需要RTC模块已正确初始化
 * 
 * @details 时间转换逻辑：
 * - tm_year从1900年开始计数，故需Time.Year-1900
 * - tm_month范围0-11，故需Time.Month-1
 * - 减去8*3600秒将北京时间(UTC+8)转换为UTC时间
 * 
 * 操作流程：
 * 1. 填充tm结构体：将用户时间转换为标准时间结构
 * 2. mktime生成时间戳：将本地时间转换为UTC秒数
 * 3. 时区修正：减去8小时（针对东八区时间输入）
 * 4. 写入RTC计数器：通过RTC_SetCounter设置当前时间
 * 5. 等待操作完成：确保寄存器写入成功
 */
void MyRTC_SetTime(void)
{
    time_t time_cnt;
    struct tm time_date;  
    
    /* 将用户自定义时间转换为标准tm结构 */
    time_date.tm_year = Time.Year - 1900;  // 年份从1900开始计数
    time_date.tm_mon = Time.Month - 1;    // 月份0-11对应1-12月
    time_date.tm_mday = Time.Day;
    time_date.tm_hour = Time.Hour;
    time_date.tm_min = Time.Minute;
    time_date.tm_sec = Time.Second;
	time_date.tm_wday = Time.wday;	
    
    /* 生成Unix时间戳并进行时区修正（假设输入时间为北京时间UTC+8） */
    time_cnt = mktime(&time_date) - 8*60*60;  // 转换为UTC时间戳
    
    /* 写入RTC计数器 */
    RTC_SetCounter(time_cnt);     // 设置RTC计数器值为UTC时间戳
    RTC_WaitForLastTask();        // 等待寄存器写入完成
}


/**
 * @brief 从RTC读取当前时间并转换为本地时间结构
 * @note 核心流程：
 * 1. 读取RTC计数器值(UTC时间戳)
 * 2. 增加8小时时区修正(适用于东八区)
 * 3. 转换为tm结构体
 * 4. 填充自定义Time结构
 */
void MyRTC_ReadTime(void)
{
    time_t time_cnt;
    struct tm time_date;
    
    /* 获取RTC计数器值并做时区修正 */
    time_cnt = RTC_GetCounter() + 8*60*60; // UTC+8转换为北京时间
    
    /* 将时间戳转换为本地时间结构 */
    time_date = *localtime(&time_cnt);     // 使用标准库时间转换函数
    
    /* 转换tm结构到自定义时间格式 */
    Time.Year = time_date.tm_year + 1900;  // tm_year从1900开始计数
    Time.Month = time_date.tm_mon + 1;     // tm_mon范围0-11对应1-12月
    Time.Day = time_date.tm_mday;
    Time.Hour = time_date.tm_hour;
    Time.Minute = time_date.tm_min;
    Time.Second = time_date.tm_sec;
	Time.wday = time_date.tm_wday;
}

