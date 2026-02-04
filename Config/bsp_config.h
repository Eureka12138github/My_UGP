/**
 ******************************************************************************
 * @file    bsp_config.h
 * @author  Eureka
 * @brief   STM32F103C8T6 最小系统板外设硬件抽象层（BSP）配置
 *
 *          定义所有外设（OLED、PMS7003、ESP8266、XM7903）的引脚、时钟、
 *          串口、DMA 等底层连接参数。修改此文件即可适配不同硬件布局。
 ******************************************************************************
 */

#ifndef BSP_CONFIG_H
#define BSP_CONFIG_H

/* === 标准库依赖 === */
#include "stm32f10x.h"
#include <string.h>
#include <math.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>

/* ============================================================================ */
/*                            OLED 显示屏配置                                   */
/* ============================================================================ */
/** @defgroup OLED_Config OLED 引脚与通信参数 */
/** @{ */
#define OLED_SCL_PIN        GPIO_Pin_8          /*!< I2C 时钟线 SCL 引脚 */
#define OLED_SDA_PIN        GPIO_Pin_9          /*!< I2C 数据线 SDA 引脚 */
#define OLED_GPIO_PORT      GPIOB               /*!< SCL/SDA 所在 GPIO 端口 */
#define OLED_GPIO_CLK       RCC_APB2Periph_GPIOB/*!< 对应 GPIO 时钟使能位 */

#define OLED_I2C_ADDR       0x78                /*!< OLED 模块 I2C 地址（7位左移后为 0x78） */
/** @} */


/* ============================================================================ */
/*                         PMS7003 颗粒物传感器配置                             */
/* ============================================================================ */
/** @defgroup PMS7003_Config PMS7003 通信与控制引脚 */
/** @{ */

/* --- UART 通信接口 --- */
#define PMS7003_USART               USART1                  /*!< 使用 USART1 与 PMS7003 通信 */
#define PMS7003_USART_CLK           RCC_APB2Periph_USART1   /*!< USART1 时钟 */
#define PMS7003_UART_GPIO_PORT      GPIOA                   /*!< TX/RX 引脚所在端口 */
#define PMS7003_UART_GPIO_CLK       RCC_APB2Periph_GPIOA    /*!< GPIOA 时钟 */
#define PMS7003_TX_PIN              GPIO_Pin_9              /*!< USART1_TX → PMS7003_RX */
#define PMS7003_RX_PIN              GPIO_Pin_10             /*!< USART1_RX ← PMS7003_TX */

/* --- 控制引脚（GPIO）--- */
#define PMS7003_CTRL_GPIO_PORT      GPIOB                   /*!< 控制引脚端口 */
#define PMS7003_CTRL_GPIO_CLK       RCC_APB2Periph_GPIOB    /*!< GPIOB 时钟 */
#define PMS7003_EN_PIN              GPIO_Pin_6              /*!< PM_SET：高电平使能传感器 */
#define PMS7003_RST_PIN             GPIO_Pin_5              /*!< PM_RESET：低电平复位传感器 */

/* --- DMA 接收配置（用于高效接收 32 字节数据包）--- */
#define PMS7003_DMA_CHANNEL         DMA1_Channel5           /*!< USART1_RX 对应 DMA1 Channel5 */
#define PMS7003_DMA_IRQ             DMA1_Channel5_IRQn      /*!< DMA 传输完成中断号 */

/* --- 协议参数 --- */
#define PMS7003_BAUDRATE            9600U                   /*!< 固定波特率，不可更改 */
#define PMS7003_PACKET_LEN          32U                     /*!< 标准数据包长度（含校验） */

/** @} */


/* ============================================================================ */
/*                          ESP8266 Wi-Fi 模块配置                              */
/* ============================================================================ */
/** @defgroup ESP8266_Config ESP8266 串口与复位引脚 */
/** @{ */

/* --- 选择通信串口（三选一）--- */
//#define ESP8266_USE_USART1     // 若使用 PA9/PA10 (USART1)
#define ESP8266_USE_USART2         // 若使用 PA2/PA3 (USART2) ← 当前启用
//#define ESP8266_USE_USART3     // 若使用 PB10/PB11 (USART3)

/* --- 自动映射所选串口 --- */
#ifdef ESP8266_USE_USART1
    #define BSP_ESP8266_USARTx      USART1
#elif defined(ESP8266_USE_USART2)
    #define BSP_ESP8266_USARTx      USART2
#elif defined(ESP8266_USE_USART3)
    #define BSP_ESP8266_USARTx      USART3
#else
    #error "One of ESP8266_USE_USART1/2/3 must be defined!"
#endif

#define BSP_ESP8266_BAUDRATE        115200U                 /*!< AT 固件常用波特率 */

/* --- 硬件复位引脚（可选，但推荐使用）--- */
#define BSP_ESP8266_GPIO_CLK        RCC_APB2Periph_GPIOA    /*!< RST 引脚所在 GPIO 时钟 */
#define BSP_ESP8266_RST_PORT        GPIOA                   /*!< 复位引脚端口 */
#define BSP_ESP8266_RST_PIN         GPIO_Pin_4              /*!< ESP8266_RST 连接至此引脚 */

/** @} */


/* ============================================================================ */
/*                        XM7903 噪音传感器配置                             */
/* ============================================================================ */
/** @defgroup XM7903_Config XM7903 串口与 DMA */
/** @{ */

#define XM7903_USART                USART3                  /*!< 使用 USART3 通信 */
#define XM7903_USART_CLK            RCC_APB1Periph_USART3   /*!< USART3 时钟（APB1） */

#define XM7903_GPIO_PORT            GPIOB                   /*!< TX/RX 引脚端口 */
#define XM7903_GPIO_CLK             RCC_APB2Periph_GPIOB    /*!< GPIOB 时钟 */
#define XM7903_TX_PIN               GPIO_Pin_10             /*!< USART3_TX → XM7903_RX */
#define XM7903_RX_PIN               GPIO_Pin_11             /*!< USART3_RX ← XM7903_TX */

/* --- DMA 接收配置（注意：通道变更需同步修改中断服务函数名！）--- */
#define XM7903_DMA_CHANNEL          DMA1_Channel3           /*!< USART3_RX → DMA1 Channel3 */
#define XM7903_DMA_CLK              RCC_AHBPeriph_DMA1      /*!< DMA 控制器时钟 */
#define XM7903_DMA_TC_IT            DMA1_IT_TC3             /*!< 传输完成中断标志 */
#define XM7903_DMA_IRQn             DMA1_Channel3_IRQn      /*!< 对应中断号 */

#define XM7903_BAUDRATE             9600                    /*!< 固定波特率 */

/** @} */

#endif /* BSP_CONFIG_H */
