#ifndef SENSORS_DATA_H
#define SENSORS_DATA_H
#include "stm32f10x.h"                  // Device header
#include "pms7003_drv.h"
#include "xm7903_drv.h"

typedef struct {
    PM_SensorData   pm;
    XM7903_Data_t   noise;
    uint16_t        temp;
    uint16_t        humi;
    uint16_t        dust_limit;
    uint16_t        noise_limit;
} SensorsData_t;

void SensorsData_Update_PM(const PM_SensorData* pm);
void SensorsData_Update_Noise(const XM7903_Data_t* noise);
void SensorsData_Update_Temp_Humi(const u16* temp,const u16* humi);
const SensorsData_t* SensorsData_Get(void);

extern SensorsData_t g_sensor_data;
//调用函数更新各个数据，


#endif
