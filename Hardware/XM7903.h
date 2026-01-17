// xm7903.h （放在 Drivers/ 或 include/ 目录）
#ifndef __XM7903_H
#define __XM7903_H

#include <stdint.h>
#include <stdbool.h>
#include "CRC16.h"
#include "bsp_xm7903.h"
typedef struct {
    float noise_db;      // 噪声值，单位 dB
    bool valid;          // 是否有效（CRC 正确且格式合法）
} XM7903_Data_t;

// 函数声明
XM7903_Data_t XM7903_Parse(const uint8_t *frame);

#endif
