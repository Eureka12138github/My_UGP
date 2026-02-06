#include "onenet_handler.h"

/**
 * @brief 数据上报任务（发送方向）
 *
 * 定期调用 OneNet_SendData() 上报传感器数据。
 * - 成功：清零失败计数
 * - 失败：累计失败次数，连续 ≥3 次则记录通信错误并系统复位
 *
 * @note 当前策略为“快速失败+复位”，未来可扩展为非阻塞重连机制。
 */
void Data_Send(void) {
    static uint8_t send_fail_count = 0;

    if (OneNet_SendData() == ONENET_OK) {
        send_fail_count = 0; // 发送成功，重置计数器
    } else {
        send_fail_count++;

        if (send_fail_count >= 3) {
            ErrorType(ENV_COMM_DATA_TRANSMISSION_FAILURE);
            NVIC_SystemReset(); // 触发看门狗或硬复位
        }
    }
}

/**
 * @brief 网络数据接收与处理任务（接收方向）
 *
 * 非阻塞轮询 ESP8266 的 IPD 帧，仅处理来自 OneNET 平台的控制指令：
 * - 解析 MQTT 消息（属性设置、上报响应等）
 * - 根据返回状态更新 UI（如成功指示）、记录错误或应用配置
 * - 其他类型帧（如 ACK、AT 响应）静默丢弃
 */
void Handle_Network_Data(void) {
    static uint8_t parse_error_count = 0;

    esp8266_ipd_frame_t frame = ESP8266_GetIPD(0); // 非阻塞获取
    if (!frame.valid || frame.type != ESP8266_IPD_TYPE_ONENET_CMD) {
        return; // 无有效数据或非 OneNET 控制帧，直接返回
    }

    ONENET_LOG_PARSE("Raw frame len: %u", (unsigned)strlen((char*)frame.data));

    uint8_t status = OneNet_RevPro((unsigned char*)frame.data);

    switch (status) {
        case ONENET_PARSE_ERR:
            parse_error_count++;
            if (parse_error_count >= 3) {
                ErrorType(ENV_COMM_DATA_RECEPTION_FAILURE);
            }
            ONENET_LOG_PARSE("❌ 协议或 JSON 解析失败");
            break;

        case ONENET_POST_SUCCESS:
            Trigger_Success_Indicator(); // 触发 OLED 成功闪烁提示
            ONENET_LOG_PARSE("✅ 平台已响应");
            parse_error_count = 0;
            break;

        case ONENET_POST_FAILED:
            ONENET_LOG_PARSE("⚠️ 上报被平台拒绝，请检查物模型配置");
            parse_error_count = 0; // 不视为底层通信故障
            break;

        case ONENET_SET_HANDLED:
            ONENET_LOG_PARSE("🔧 已应用远程配置");
            parse_error_count = 0;
            break;

        case ONENET_OK:
        default:
            parse_error_count = 0; // 其他合法消息（如 PUBACK/SUBACK）
            break;
    }
}
