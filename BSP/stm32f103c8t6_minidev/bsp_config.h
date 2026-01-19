// BSP/stm32f103c8t6_minidev/bsp_config.h
#ifndef BSP_CONFIG_H
#define BSP_CONFIG_H
#include "stm32f10x.h"       
#include <string.h>
#include <math.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>

// OLED 引脚定义
#define OLED_SCL_PIN   GPIO_Pin_8
#define OLED_SDA_PIN   GPIO_Pin_9
#define OLED_GPIO_PORT GPIOB
#define OLED_GPIO_CLK  RCC_APB2Periph_GPIOB   

// OLED I2C 地址（通常 0x78）
#define OLED_I2C_ADDR  0x78



/* ========== PMS7003 通信接口 ========== */
#define PMS7003_USART               USART1
#define PMS7003_USART_CLK           RCC_APB2Periph_USART1
#define PMS7003_UART_GPIO_PORT      GPIOA          // TX/RX 共用 GPIOA
#define PMS7003_UART_GPIO_CLK       RCC_APB2Periph_GPIOA
#define PMS7003_TX_PIN              GPIO_Pin_9
#define PMS7003_RX_PIN              GPIO_Pin_10

/* ========== PMS7003 控制引脚 ========== */
#define PMS7003_CTRL_GPIO_PORT      GPIOB
#define PMS7003_CTRL_GPIO_CLK       RCC_APB2Periph_GPIOB
#define PMS7003_EN_PIN              GPIO_Pin_6     // PM_SET (使能)
#define PMS7003_RST_PIN             GPIO_Pin_5     // PM_RESET

/* ========== PMS7003 DMA 配置（改通道必须改对应的中断函数名称） ========== */
#define PMS7003_DMA_CHANNEL         DMA1_Channel5
#define PMS7003_DMA_IRQ             DMA1_Channel5_IRQn

/* ========== PMS7003 协议参数 ========== */
#define PMS7003_BAUDRATE            9600U
#define PMS7003_PACKET_LEN          32U

/* ==================== USART2 - ESP8266 ==================== */
#define BSP_ESP8266_USART               USART2
#define BSP_ESP8266_USART_CLK           RCC_APB1Periph_USART2
#define BSP_ESP8266_GPIO_CLK            RCC_APB2Periph_GPIOA
#define BSP_ESP8266_TX_PIN              GPIO_Pin_2
#define BSP_ESP8266_TX_PORT             GPIOA
#define BSP_ESP8266_RX_PIN              GPIO_Pin_3
#define BSP_ESP8266_RX_PORT             GPIOA
#define BSP_ESP8266_RST_PIN             GPIO_Pin_4
#define BSP_ESP8266_RST_PORT            GPIOA
#define BSP_ESP8266_BAUDRATE            115200U

// === XM7903 硬件连接配置 ===
#define XM7903_USART               USART3
#define XM7903_USART_CLK           RCC_APB1Periph_USART3

#define XM7903_GPIO_PORT           GPIOB
#define XM7903_GPIO_CLK            RCC_APB2Periph_GPIOB
#define XM7903_TX_PIN              GPIO_Pin_10
#define XM7903_RX_PIN              GPIO_Pin_11

//如果改了DMA通道，对应的中断函数也必须改，就是bsp_xm7903.c中的 DMA1_Channel3_IRQHandler
#define XM7903_DMA_CHANNEL         DMA1_Channel3
#define XM7903_DMA_CLK             RCC_AHBPeriph_DMA1
#define XM7903_DMA_TC_IT           DMA1_IT_TC3
#define XM7903_DMA_IRQn            DMA1_Channel3_IRQn

#define XM7903_BAUDRATE            9600

#endif


