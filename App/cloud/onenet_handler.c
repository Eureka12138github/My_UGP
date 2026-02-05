#include "onenet_handler.h"



/**
 * @brief 数据发送函数，负责调用OneNet发送数据，并处理成功/失败情况：
 *        - 成功时通过OLED闪烁指示
 *        - 失败超过阈值则记录错误信息、延时并触发看门狗复位
 */
 //这是发
void Data_Send(void) {
	//数据无法发送尝试重连而不是直接复位？：（未实现）
										//状态机非阻塞地检查 MQTT 状态；
										//定时触发重连逻辑
    static u8 Send_Data_Error = 0; // 静态变量用于记录连续发送失败次数

    if (!OneNet_SendData()) {

        Send_Data_Error = 0; // 发送成功，清空失败计数器
    } else {
        // 数据发送失败，增加失败计数
        Send_Data_Error++;

        // 如果失败次数 >= 3，则进行错误处理
        if (Send_Data_Error >= 3) {
            // 错误处理流程：
            // 1. 设置错误类型为“环境通信数据传输失败”
            // 2. 获取当前时间
            // 3. 存储错误信息到FLASH中
//            ErrorType(ENV_COMM_DATA_TRANSMISSION_FAILURE);
//            NVIC_SystemReset();
        }
    }
}


//这是收
void Handle_Network_Data(void)
{
    static uint8_t error_count = 0;

    esp8266_ipd_frame_t frame = ESP8266_GetIPD(0); // 非阻塞

    if (!frame.valid) {
        return; // 无数据或无效帧
    }

    // ✅ 只处理 OneNET 控制指令
    if (frame.type == ESP8266_IPD_TYPE_ONENET_CMD)
    {
        u8 status = OneNet_RevPro((unsigned char*)frame.data);

        switch (status)
        {
            case ONENET_PARSE_ERR:
                // 协议或 JSON 解析失败，视为通信异常
                error_count++;
                if (error_count >= 3) {
                    ErrorType(ENV_COMM_DATA_RECEPTION_FAILURE);
                }
                break;

            case ONENET_POST_SUCCESS:
                // 平台确认 property/post 成功
        // 数据发送成功：OLED局部刷新，显示一次闪烁提示
				Trigger_Success_Indicator();
                break;

            case ONENET_POST_FAILED:
                // 平台拒绝上报（如参数非法、设备未激活等）
                error_count = 0; // 不视为底层通信错误，但需关注
                ONENET_LOG_PARSE("⚠️ 上报被平台拒绝，请检查物模型配置");
                // 可选：触发告警、记录失败次数、暂停上报等
                break;

            case ONENET_SET_HANDLED:
                // 成功处理了远程配置（如阈值更新）
                error_count = 0;
                ONENET_LOG_PARSE("🔧 已应用远程配置");
                // 可选：保存配置后重启传感器校准等
                break;

            case ONENET_OK:
            default:
                // 其他正常 MQTT 消息（如 PUBACK、SUBACK、未知 topic）
                error_count = 0;
                // 通常无需额外操作，静默处理即可
                break;
        }
    }
    // else: ACK、AT 响应或其他类型帧，静默丢弃，不计入错误
}
