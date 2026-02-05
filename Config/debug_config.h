/**
 ******************************************************************************
 * @file    debug_config.h
 * @author  Eureka
 * @brief   调试日志配置头文件
 *
 *          提供按模块开关的日志宏（如 ESP8266、OneNET），支持条件编译。
 *          所有日志通过 USART_DEBUG 输出，未启用时零开销。
 *
 * @note    使用方法：
 *          1. 在本文件顶部取消注释对应 DEBUG 宏以启用模块日志
 *          2. 在代码中调用 LOG_XXX("msg: %d", val)
 *          3. 确保 bsp_usart.h 中已初始化 USART_DEBUG
 ******************************************************************************
 */

#ifndef __DEBUG_CONFIG_H__
#define __DEBUG_CONFIG_H__

#include "bsp_usart.h"  // 确保 Serial_Printf 可用

/* ============================================================================ */
/*                     【配置区】按需启用调试模块                                */
/* ============================================================================ */

// ── OneNET 平台调试 ───────────────────────────────────────
// #define ONENET_DEBUG_SEND    // 数据帧发送（含 Hex）
//// #define ONENET_DEBUG_CONN    // 连接状态（心跳、断连等）
// #define ONENET_DEBUG_PARSE   // （预留）下行指令解析
// #define ONENET_DEBUG_HB      // （预留）心跳细节

// ── ESP8266 驱动调试 ──────────────────────────────────────
// #define ESP8266_DEBUG_CMD    // 打印发送的 AT 指令及期望响应
// #define ESP8266_DEBUG_FAIL   // 打印超时/失败详情（含实际收到数据）
// #define ESP8266_DEBUG_INIT   // 打印初始化各阶段日志
// #define ESP8266_DEBUG_IPD    // 打印 +IPD 接收内容（仅调试通信问题时启用！）

// ── RTC 实时时钟调试 ───────────────────────────────────────
// #define RTC_DEBUG_INIT   // 启用 RTC 初始化日志

/* ============================================================================ */
/*                     【系统配置】调试串口定义                                  */
/* ============================================================================ */

/**
 * @brief 调试日志输出串口（必须已在 bsp_usart.c 中初始化）
 *
 * 示例：若使用 USART3 作为调试口，则在 bsp_usart.h 中应有：
 *       #define USART_DEBUG USART3
 */
#ifndef USART_DEBUG
    #error "USART_DEBUG is undefined! Please specify the debug serial port in bsp_usart.h or here."
#endif

/* ============================================================================ */
/*                     【日志宏定义】自动条件编译                                */
/* ============================================================================ */

// ── OneNET 模块 ───────────────────────────────────────────
#ifdef ONENET_DEBUG_SEND
  #define ONENET_LOG_SEND(fmt, ...) \
      Serial_Printf(USART_DEBUG, "[ONENET][SEND] " fmt "\r\n", ##__VA_ARGS__)
#else
  #define ONENET_LOG_SEND(fmt, ...) ((void)0)
#endif

#ifdef ONENET_DEBUG_CONN
  #define ONENET_LOG_CONN(fmt, ...) \
      Serial_Printf(USART_DEBUG, "[ONENET][CONN] " fmt "\r\n", ##__VA_ARGS__)
#else
  #define ONENET_LOG_CONN(fmt, ...) ((void)0)
#endif

#ifdef ONENET_DEBUG_PARSE
  #define ONENET_LOG_PARSE(fmt, ...) \
      Serial_Printf(USART_DEBUG, "[ONENET][PARSE] " fmt "\r\n", ##__VA_ARGS__)
#else
  #define ONENET_LOG_PARSE(fmt, ...) ((void)0)
#endif

#ifdef ONENET_DEBUG_HB
  #define ONENET_LOG_HB(fmt, ...) \
      Serial_Printf(USART_DEBUG, "[ONENET][HB] " fmt "\r\n", ##__VA_ARGS__)
#else
  #define ONENET_LOG_HB(fmt, ...) ((void)0)
#endif


// ── ESP8266 模块 ───────────────────────────────────────────
#ifdef ESP8266_DEBUG_CMD
  #define ESP8266_LOG_CMD(fmt, ...) \
      Serial_Printf(USART_DEBUG, "[ESP8266][CMD] " fmt "\r\n", ##__VA_ARGS__)
#else
  #define ESP8266_LOG_CMD(fmt, ...) ((void)0)
#endif

#ifdef ESP8266_DEBUG_FAIL
  #define ESP8266_LOG_FAIL(fmt, ...) \
      Serial_Printf(USART_DEBUG, "[ESP8266][FAIL] " fmt "\r\n", ##__VA_ARGS__)
#else
  #define ESP8266_LOG_FAIL(fmt, ...) ((void)0)
#endif

#ifdef ESP8266_DEBUG_INIT
  #define ESP8266_LOG_INIT(fmt, ...) \
      Serial_Printf(USART_DEBUG, "[ESP8266][INIT] " fmt "\r\n", ##__VA_ARGS__)
#else
  #define ESP8266_LOG_INIT(fmt, ...) ((void)0)
#endif

#ifdef ESP8266_DEBUG_IPD
  #define ESP8266_LOG_IPD(fmt, ...) \
      Serial_Printf(USART_DEBUG, "[ESP8266][IPD] " fmt "\r\n", ##__VA_ARGS__)
#else
  #define ESP8266_LOG_IPD(fmt, ...) ((void)0)
#endif
  
  
// ── RTC 模块 ───────────────────────────────────────────────
#ifdef RTC_DEBUG_INIT
  #define RTC_LOG_INIT(fmt, ...) \
      Serial_Printf(USART_DEBUG, "[RTC] " fmt "\r\n", ##__VA_ARGS__)
#else
  #define RTC_LOG_INIT(fmt, ...) ((void)0)
#endif  


/* ============================================================================ */
/*                     【扩展指南】新增模块示例                                  */
/* ============================================================================ */
/*
// 1. 在【配置区】添加：
// #define MYMODULE_DEBUG_FOO

// 2. 在此处添加宏：
#ifdef MYMODULE_DEBUG_FOO
  #define MYMODULE_LOG_FOO(fmt, ...) \
      Serial_Printf(USART_DEBUG, "[MYMODULE][FOO] " fmt "\r\n", ##__VA_ARGS__)
#else
  #define MYMODULE_LOG_FOO(fmt, ...) ((void)0)
#endif
*/

#endif /* __DEBUG_CONFIG_H__ */
