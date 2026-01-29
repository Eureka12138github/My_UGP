// debug_config.h

#ifndef __DEBUG_CONFIG_H__
#define __DEBUG_CONFIG_H__

#include "bsp_usart.h"  // 确保 UsartPrintf 可见

// ====== 模块调试开关（按需启用）======
// #define ONENET_DEBUG_SEND    // 数据发送
// #define ONENET_DEBUG_CONN    // 连接管理
// #define ONENET_DEBUG_PARSE   // （预留）
// #define ONENET_DEBUG_HB      // （预留）



// ====== ESP8266 调试开关（按需启用）======
// #define ESP8266_DEBUG_CMD    // 打印发送的 AT 指令和期望响应
// #define ESP8266_DEBUG_FAIL   // 打印超时/失败详情（含实际收到的数据）
// #define ESP8266_DEBUG_INIT	// 打印启用初始化流程日志



// ====== 统一日志宏（带模块标签）======
#ifdef ONENET_DEBUG_SEND
  #define ONENET_LOG_SEND(fmt, ...) \
      Serial_Printf(USART_DEBUG, "[ONENET][SEND] " fmt "\r\n", ##__VA_ARGS__)
#else
  #define ONENET_LOG_SEND(fmt, ...)
#endif

#ifdef ONENET_DEBUG_CONN
  #define ONENET_LOG_CONN(fmt, ...) \
      Serial_Printf(USART_DEBUG, "[ONENET][CONN] " fmt "\r\n", ##__VA_ARGS__)
#else
  #define ONENET_LOG_CONN(fmt, ...)
#endif

#ifdef ONENET_DEBUG_PARSE
  #define ONENET_LOG_PARSE(fmt, ...) \
      Serial_Printf(USART_DEBUG, "[ONENET][PARSE] " fmt "\r\n", ##__VA_ARGS__)
#else
  #define ONENET_LOG_PARSE(fmt, ...)
#endif



// ====== 统一日志宏 ======
#ifdef ESP8266_DEBUG_CMD
  #define ESP8266_LOG_CMD(fmt, ...) \
      Serial_Printf(USART_DEBUG, "[ESP8266][CMD] " fmt "\r\n", ##__VA_ARGS__)
#else
  #define ESP8266_LOG_CMD(fmt, ...)
#endif

#ifdef ESP8266_DEBUG_FAIL
  #define ESP8266_LOG_FAIL(fmt, ...) \
      Serial_Printf(USART_DEBUG, "[ESP8266][FAIL] " fmt "\r\n", ##__VA_ARGS__)
#else
  #define ESP8266_LOG_FAIL(fmt, ...)
#endif

#ifdef ESP8266_DEBUG_INIT
  #define ESP8266_LOG_INIT(fmt, ...) \
      Serial_Printf(USART_DEBUG, "[ESP8266][INIT] " fmt "\r\n", ##__VA_ARGS__)
#else
  #define ESP8266_LOG_INIT(fmt, ...)
#endif



// 预留：新增模块时只需复制上面两段并改名即可
// 例如：
/*
#ifdef ONENET_DEBUG_PARSE
  #define ONENET_LOG_PARSE(fmt, ...) \
      Serial_Printf(USART_DEBUG, "[ONENET][PARSE] " fmt "\r\n", ##__VA_ARGS__)
#else
  #define ONENET_LOG_PARSE(fmt, ...)
#endif
*/

#endif
