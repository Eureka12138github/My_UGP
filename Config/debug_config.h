// debug_config.h

#ifndef __DEBUG_CONFIG_H__
#define __DEBUG_CONFIG_H__

#include "bsp_usart.h"  // 确保 UsartPrintf 可见

// ====== 模块调试开关（按需启用）======
// #define ONENET_DEBUG_SEND    // 数据发送
// #define ONENET_DEBUG_CONN    // 连接管理
// #define ONENET_DEBUG_PARSE   // （预留）
// #define ONENET_DEBUG_HB      // （预留）

// ====== 统一日志宏（带模块标签）======
#ifdef ONENET_DEBUG_SEND
  #define ONENET_LOG_SEND(fmt, ...) \
      UsartPrintf(USART_DEBUG, "[ONENET][SEND] " fmt "\r\n", ##__VA_ARGS__)
#else
  #define ONENET_LOG_SEND(fmt, ...)
#endif

#ifdef ONENET_DEBUG_CONN
  #define ONENET_LOG_CONN(fmt, ...) \
      UsartPrintf(USART_DEBUG, "[ONENET][CONN] " fmt "\r\n", ##__VA_ARGS__)
#else
  #define ONENET_LOG_CONN(fmt, ...)
#endif

// 预留：新增模块时只需复制上面两段并改名即可
// 例如：
/*
#ifdef ONENET_DEBUG_PARSE
  #define ONENET_LOG_PARSE(fmt, ...) \
      UsartPrintf(USART_DEBUG, "[ONENET][PARSE] " fmt "\r\n", ##__VA_ARGS__)
#else
  #define ONENET_LOG_PARSE(fmt, ...)
#endif
*/

#endif
