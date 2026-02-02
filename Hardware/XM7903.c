// XM7903 数据解析
#include "xm7903.h"

// 典型使用方式（由周期性任务调用，如每 1000ms）：
// ┌─────────────────────────────────────────────────────────────┐
// │ BSP_XM7903_SendQuery();      // 发送 MODBUS 查询帧          │
// │ BSP_XM7903_StartReceive();   // 启动 7 字节 DMA 接收        │
// │                                                             │
// │ if (xm7903_rx_ready) {                                      │
// │     xm7903_rx_ready = false;                                │
// │     g_xm7903_data = XM7903_Parse(BSP_XM7903_GetRxBuffer()); │
// │ } else {                                                    │
// │     g_xm7903_data.valid = false;                            │
// │     g_xm7903_data.noise_db = 0.0f;                          │
// │ }                                                           │
// └─────────────────────────────────────────────────────────────┘
//
// 上层通过 g_xm7903_data.valid 判断数据有效性，
// 通过 g_xm7903_data.noise_db 获取噪声值（仅当 valid 为 true 时有效）。

// 注意：
// - 传感器响应时间 << 任务周期（实测 <500ms），故无需“忙”标志；
// - 每次任务强制重发新查询，上一轮未处理的响应会被覆盖或丢弃；



// 启动阶段临时设 valid = true，防止系统初始化期间（如 WIFI/ONENET 连接）
// 因尚未收到传感器数据而误触发 Noise_Data_Error 任务。
// 首次 XM7903_Task() 执行后，该字段将被真实通信结果覆盖。
volatile XM7903_Data_t g_xm7903_data = {
    .noise_db = 0.0f,
    .valid = true
};	

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


/**
 * @brief XM7903传感器数据采集任务
 * 
 * 执行一次完整的传感器数据读取流程：
 * 1. 发送MODBUS查询命令
 * 2. 启动DMA接收等待响应
 * 3. 检查接收状态并处理响应数据
 * 
 * 该函数应在合适的时间间隔内定期调用，以实现连续的数据采集
 * 
 * @note 该函数是非阻塞的，不会等待传感器响应完成
 *       响应数据通过DMA异步接收，下次调用时处理
 * 
 * @see BSP_XM7903_SendQuery
 * @see BSP_XM7903_StartReceive
 * @see BSP_XM7903_GetRxBuffer
 * @see XM7903_Parse
 */
void XM7903_Task(void)
{
    BSP_XM7903_SendQuery();
    BSP_XM7903_StartReceive();

    if (xm7903_rx_ready) {
        xm7903_rx_ready = false;
        const uint8_t *buf = BSP_XM7903_GetRxBuffer();
        g_xm7903_data = XM7903_Parse(buf); // 内部已处理 CRC 失败情况
    } else {
        // 未收到响应：明确标记为无效，并清零数值
        g_xm7903_data.valid = false;
//        g_xm7903_data.noise_db = 0.0f;
    }
}
