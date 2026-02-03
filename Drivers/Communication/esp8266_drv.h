/**
 ******************************************************************************
 * @file    esp8266_drv.h
 * @author  Eureka
 * @date    2026-01-29
 * @brief   ESP8266 AT 指令驱动接口声明
 *
 *          提供硬件初始化、AT 指令交互、数据收发及 +IPD 帧解析功能。
 *          支持 OneNET 平台对接，采用异步非阻塞接收模型。
 ******************************************************************************
 */

#ifndef __ESP8266_DRV_H
#define __ESP8266_DRV_H

/* === 标准库依赖 === */
#include <stdint.h>
#include <stdbool.h>   // 使用标准 bool 类型（替代 _Bool）
#include <ctype.h>     // isdigit()
#include <stdlib.h>    // atoi()

/* === 平台与 BSP 依赖 === */
#include "stm32f10x.h"
#include "bsp_usart.h"
#include "bsp_config.h"
#include "debug_config.h"
#include "project_secrets.h"
#include "bsp_delay.h"
#include "OLED.h"

/* === 配置宏 === */
#define IPD_BUFFER_SIZE 256U  /*!< +IPD 数据载荷最大缓冲区大小 */

/* === 类型定义 === */

/**
 * @brief ESP8266 +IPD 接收数据类型枚举
 */
typedef enum {
    ESP8266_IPD_TYPE_UNKNOWN = 0,   /*!< 未知或无效数据 */
    ESP8266_IPD_TYPE_ACK,           /*!< 平台 ACK 响应（如 0x40 0x02 0x00 0x0A） */
    ESP8266_IPD_TYPE_ONENET_CMD,    /*!< OneNET 下发的 JSON 控制指令 */
    ESP8266_IPD_TYPE_CUSTOM         /*!< 其他自定义业务数据 */
} esp8266_ipd_type_t;

/**
 * @brief +IPD 帧解析结果结构体
 */
typedef struct {
    const uint8_t        *data;     /*!< 指向内部静态缓冲区（只读，有效期至下次调用） */
    uint16_t              len;      /*!< 数据长度（字节） */
    esp8266_ipd_type_t    type;     /*!< 业务类型 */
    bool                  valid;    /*!< 是否为有效帧（true 表示成功解析） */
} esp8266_ipd_frame_t;

/* === 公共 API 声明 === */

/**
 * @brief 初始化 ESP8266 所需的底层硬件（串口 + 复位引脚）
 */
void ESP8266_HardwareInit(void);

/**
 * @brief 发送 AT 指令并等待指定响应子串（阻塞式，超时 2s）
 * @param[in] cmd 要发送的 AT 指令（必须以 "\r\n" 结尾）
 * @param[in] res 期望在响应中出现的关键子串（如 "OK"）
 * @retval true  成功匹配到响应
 * @retval false 超时或未匹配
 */
bool ESP8266_SendCmd(const char *cmd, const char *res);

/**
 * @brief 完整初始化 ESP8266（AT 测试 → STA 模式 → DHCP → WiFi 连接）
 * @retval 0 成功
 * @retval 1~4 对应各阶段失败（见函数实现）
 */
uint8_t ESP8266_Init(void);

/**
 * @brief 提交应用数据到 ESP8266（不等待网络确认）
 * @param[in] data 待发送数据指针
 * @param[in] len  数据长度（字节）
 * @retval true  提交失败（未收到 ">"）
 * @retval false 提交成功（数据已发出，响应由 GetIPD 异步处理）
 */
bool ESP8266_SendData(const unsigned char *data, uint16_t len);

/**
 * @brief 非阻塞/阻塞式解析 +IPD 数据帧
 * @param[in] timeout_ms 超时时间（ms），0 = 非阻塞
 * @return 解析结果结构体（.valid 表示是否有效）
 */
esp8266_ipd_frame_t ESP8266_GetIPD(uint16_t timeout_ms);

#endif /* __ESP8266_DRV_H */
