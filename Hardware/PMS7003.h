#ifndef __PMS7003_H
#define __PMS7003_H
#include "stm32f10x.h"                  // Device header
#include <stdbool.h>
#include <stddef.h>  // 用于NULL定义
// 结构体定义（所有需要使用的文件都包含此头文件）
typedef struct {
    uint16_t pm1_0_cf1;    			// 数据1: PM1.0 CF=1
    uint16_t pm2_5_cf1;     		// 数据2: PM2.5 CF=1
    uint16_t pm10_cf1;      		// 数据3: PM10 CF=1
    uint16_t pm1_0_env;     		// 数据4: PM1.0 大气环境
    uint16_t pm2_5_env;     		// 数据5: PM2.5 大气环境
    uint16_t pm10_env;      		// 数据6: PM10 大气环境
    uint16_t particles0_3;  		// 数据7: ≥0.3μm颗粒数
    uint16_t particles0_5;  		// 数据8: ≥0.5μm颗粒数
    uint16_t particles1_0;  		// 数据9: ≥1.0μm颗粒数
    uint16_t particles2_5;  		// 数据10: ≥2.5μm颗粒数
    uint16_t particles5_0;  		// 数据11: ≥5.0μm颗粒数
    uint16_t particles10;   		// 数据12: ≥10μm颗粒数
    bool is_valid;          		// 校验通过标志
} PM_SensorData;
PM_SensorData PMS_ParseDataPacket(const uint8_t *packet, size_t packet_len);
void PMS7003_Init(void);
#endif 


//以空行结尾
