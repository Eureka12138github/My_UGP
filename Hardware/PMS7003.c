#include "PMS7003.h"
#define PARSE_DATA(packet, index) \
    ((uint16_t)((packet[index] << 8) | packet[index + 1]))
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
// 数据包解析函数（带校验和验证）
PM_SensorData PMS_ParseDataPacket(const uint8_t *packet, size_t packet_len) {
    PM_SensorData data = {0};
    data.is_valid = false; 

    // 基础校验：空指针或长度不足(此处无需进行数据校验，因为在串口1中断函数已经校验过了。)
    if (packet == NULL || packet_len < Particles10 + 2) {
        return data;
    }
    // 解析各字段
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
    data.is_valid = true;
    return data;
}
void PMS7003_Init(void){
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB,ENABLE);
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode=GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Pin=GPIO_Pin_5|GPIO_Pin_6;
	GPIO_InitStructure.GPIO_Speed=GPIO_Speed_50MHz;
	GPIO_Init(GPIOB,&GPIO_InitStructure);
	GPIO_SetBits(GPIOB,GPIO_Pin_5|GPIO_Pin_6);//表示刚开始时，B5、B6口为高电平
}
