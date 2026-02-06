#include "sensors_data.h"

SensorsData_t g_sensor_data = {
    .pm = {0},          
    .noise = {
	.noise_db = 0.0f,
    .valid = true
	},          
    .temp = 0,
    .humi = 0
};


// 各传感器专用更新函数
void SensorsData_Update_PM(const PM_SensorData* pm) {
    if (pm == NULL) return;
    
    g_sensor_data.pm = *pm;  
    
}

void SensorsData_Update_Noise(const XM7903_Data_t* noise) {
	
    if (noise == NULL) return;
    
    g_sensor_data.noise = *noise; 
}

void SensorsData_Update_Temp_Humi(const u16* temp,const u16* humi) {
	
    if (temp == NULL || humi == NULL) return;
    
    g_sensor_data.temp = *temp; 
	g_sensor_data.humi = *humi;
}

// 安全获取只读指针（外部只能读，不能改）

const SensorsData_t* SensorsData_Get(void) {
    return &g_sensor_data;  // ← 这是一个“读取”操作！
}


