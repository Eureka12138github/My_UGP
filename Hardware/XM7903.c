// XM7903 数据解析
#include "xm7903.h"

/**
 * @brief 解析XM7903传感器返回的MODBUS数据帧
 * 
 * 对接收到的7字节MODBUS RTU数据进行完整性校验(CRC)，
 * 并提取噪声测量值，转换为dB单位
 * 
 * @param frame 指向7字节MODBUS数据帧的指针
 *              格式: [地址][功能码][字节数][高字节][低字节][CRC高][CRC低]
 * 
 * @return XM7903_Data_t 包含解析结果的结构体
 *         - valid: 数据有效性标志
 *         - noise_db: 解析出的噪声值(单位dB)，仅当valid=true时有效
 * 
 * @note 输入数据帧必须为7字节MODBUS RTU格式
 *       数据转换公式: 真实值 = 寄存器值 / 10
 */
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
