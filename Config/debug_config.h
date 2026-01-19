// debug_config.h

#ifndef __DEBUG_CONFIG_H__
#define __DEBUG_CONFIG_H__

#include "bsp_usart.h"  // 确保 UsartPrintf 可见

// ==== 模块调试开关 ====
 #define ONENET_DEBUG
// #define WIFI_DEBUG
// #define SENSOR_DEBUG

// ==== 日志宏封装 ====
#ifdef ONENET_DEBUG
  #define ONENET_LOG(fmt, ...) \
      UsartPrintf(USART_DEBUG, "[ONENET] " fmt "\r\n", ##__VA_ARGS__)
#else
  #define ONENET_LOG(fmt, ...)
#endif

//// 未来加其他模块：
//#ifdef WIFI_DEBUG
//  #define WIFI_LOG(fmt, ...) \
//      UsartPrintf(USART_DEBUG, "[WIFI] " fmt "\r\n", ##__VA_ARGS__)
//#else
//  #define WIFI_LOG(fmt, ...)
//#endif

#endif
