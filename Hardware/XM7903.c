// xm7903.c
#include "xm7903.h"

// 假设你已有 crc16 函数，原型如下：
// uint16_t crc16(const uint8_t *data, uint16_t len);

XM7903_Data_t XM7903_Parse(const uint8_t *frame)
{
    XM7903_Data_t result = {0};

    if (frame == NULL) {
        result.valid = false;
        return result;
    }

    // 1. 检查基本长度（MODBUS RTU 最小为 5 字节，你的是 7）
    // 可选：检查地址和功能码（如 frame[0]==0x01, frame[1]==0x03）
    // 这里先只做 CRC 校验

    // 2. 提取并验证 CRC
    uint16_t received_crc = (frame[5] << 8) | frame[6];
    uint16_t calc_crc = crc16(frame, 5); // 前5字节

    if (received_crc != calc_crc) {
        result.valid = false;
        return result;
    }

    // 3. 提取噪声原始值（大端序）
    uint16_t raw_noise = (frame[3] << 8) | frame[4];

    // 4. 转换为 dB（根据手册：真实值 = 寄存器值 / 10）
    result.noise_db = raw_noise / 10.0f;
    result.valid = true;

    return result;
}

