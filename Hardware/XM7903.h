// xm7903.h （放在 Drivers/ 或 include/ 目录）
#ifndef __XM7903_H
#define __XM7903_H
#include "bsp_xm7903.h"
#include "CRC16.h"

typedef struct {
    float noise_db;      // 噪声值，单位 dB
    bool valid;          // 是否有效（CRC 正确且格式合法）
} XM7903_Data_t;

extern volatile XM7903_Data_t g_xm7903_data;

// 函数声明
XM7903_Data_t XM7903_Parse(const uint8_t *frame);
void XM7903_Task(void);

#endif
