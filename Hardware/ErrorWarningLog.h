#ifndef _ERRORWARNINGLOG_H_
#define _ERRORWARNINGLOG_H_
#include "stm32f10x.h"                  // Device header
#include "MyRTC.h"
#include "Store.h"
#define ERROR_TIME_ARRAY_SIZE 		3//(sizeof(ErrorTime)/sizeof(ErrorTime[0]))
#define WARNING_TIME_ARRAY_SIZE 	3//(sizeof(WarningTime)/sizeof(WarningTime[0]))
#define STORE_ERROR_DATA_START_INDEX 10	//此值不可为0
#define STORE_WARNING_DATA_START_INDEX 40	//此值不可为0
#define ENV_COMM_DATA_TRANSMISSION_FAILURE    1  // 数据无法发送
#define ENV_COMM_DATA_RECEPTION_FAILURE       2  // 数据无法接收
#define ENV_SENSOR_DUST_ANOMALY               3  // 扬尘数据异常
#define ENV_SENSOR_NOISE_ANOMALY              4  // 噪音数据异常
#define ENV_SENSOR_TEMP_HUMIDITY_ANOMALY      5  // 温湿度数据异常
#define ENV_ALERT_DUST_OVERLIMIT          1  // 扬尘过大
#define ENV_ALERT_NOISE_OVERLIMIT         2  // 噪音过高
#define ENV_ALERT_DUST_NOISE_COMBINED     3  // 扬尘过大且噪音过高
/*--------------------说明--------------------*/
/*
*数组 Store_Data[10]~Store_Data[18] 存储的是结构体 ErrorTime[0]中的信息 顺序为：年->月->日->时->分->秒->星期->错误类型->显示标志
*数组 Store_Data[19]~Store_Data[27] 存储的是结构体 ErrorTime[1]中的信息	顺序为：年->月->日->时->分->秒->星期->错误类型->显示标志	
*数组 Store_Data[28]~Store_Data[36] 存储的是结构体 ErrorTime[2]中的信息 顺序为：年->月->日->时->分->秒->星期->错误类型->显示标志
*/
/*--------------------说明--------------------*/
void ErrorType(u8 Type);
void WarningType(u8 Type);
void ReadStoreErrorTime(void);
void ReadStoreWarningTime(void);
#endif
