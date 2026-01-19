#include "PMS7003.h"
#define PARSE_DATA(packet, index) \
    ((uint16_t)((packet[index] << 8) | packet[index + 1]))
	
// 典型数据获取伪代码（由主循环或任务周期调用）：
// ┌───────────────────────────────────────┐
// │ if (pms7003_rx_ready) {               │
// │     pms7003_rx_ready = false;         │
// │     buf = BSP_PMS7003_GetRxBuffer();  │
// │     data = PMS7003_Parse(buf);        │
// │     if (data.valid) {                 │
// │        use(data.pm25, data.pm10, ...);│
// │     }                                 │
// │ }                                     │
// └───────────────────────────────────────┘	
	
//注：传感器大约每隔1秒通过串口向MCU发送一次数据
// 数据索引枚举     
typedef enum {
    PM1_0_CF1    = 4,//根据产品手册，数据1就存于数组索引4、5处,4处为高八位，5处为低八位 
    PM2_5_CF1    = 6,  
    PM10_CF1     = 8,  
    PM1_0_Env    = 10, 
    PM2_5_Env    = 12, 
    PM10_Env     = 14, 
    Particles0_3 = 16, 
    Particles0_5 = 18, 
    Particles1_0 = 20, 
    Particles2_5 = 22, 
    Particles5_0 = 24, 
    Particles10  = 26, 
} PM_DataIndex;

/**
 * @brief 解析PMS7003传感器数据包
 * @param[in] packet 指向待解析数据包的指针（必须指向完整的32字节PMS7003数据包）
 * @return PM_SensorData 解析后的传感器数据结构体
 */
PM_SensorData PMS_ParseDataPacket(const uint8_t *packet) 
{
    PM_SensorData data = {0};
    
    if (packet == NULL) {
        return data;
    }
    
    // 验证起始符和数据长度字段（协议规定：0x42 0x4D 0x00 0x1C ...）
    if (packet[0] != 0x42 || packet[1] != 0x4D || 
        packet[2] != 0x00 || packet[3] != 0x1C) {
        return data;
    }
    
    // 计算前30字节校验和
    uint16_t calculated_checksum = 0;
    for (int i = 0; i < 30; i++) {
        calculated_checksum += packet[i];
    }
    
    // 提取接收的校验和（大端序）
    uint16_t received_checksum = (packet[30] << 8) | packet[31];
    
    // 验证校验和
    if (calculated_checksum != received_checksum || calculated_checksum == 0) {
        return data;
    }
    
    // 解析有效数据
    data.pm1_0_cf1    = PARSE_DATA(packet, PM1_0_CF1);
    data.pm2_5_cf1    = PARSE_DATA(packet, PM2_5_CF1);
    data.pm10_cf1     = PARSE_DATA(packet, PM10_CF1);
    data.pm1_0_env    = PARSE_DATA(packet, PM1_0_Env);
    data.pm2_5_env    = PARSE_DATA(packet, PM2_5_Env);
    data.pm10_env     = PARSE_DATA(packet, PM10_Env);
    data.particles0_3 = PARSE_DATA(packet, Particles0_3);
    data.particles0_5 = PARSE_DATA(packet, Particles0_5);
    data.particles1_0 = PARSE_DATA(packet, Particles1_0);
    data.particles2_5 = PARSE_DATA(packet, Particles2_5);
    data.particles5_0 = PARSE_DATA(packet, Particles5_0);
    data.particles10  = PARSE_DATA(packet, Particles10);
    data.is_valid     = true;
    
    return data;
}

