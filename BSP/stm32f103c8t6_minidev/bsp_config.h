// BSP/stm32f103c8t6_minidev/bsp_config.h
#ifndef BSP_CONFIG_H
#define BSP_CONFIG_H

// OLED 引脚定义（根据你的实际接线）
#define OLED_SCL_PIN   GPIO_Pin_8
#define OLED_SDA_PIN   GPIO_Pin_9
#define OLED_GPIO_PORT GPIOB
#define OLED_GPIO_CLK  RCC_APB2Periph_GPIOB   

// OLED I2C 地址（通常 0x78）
#define OLED_I2C_ADDR  0x78

#ifndef __BSP_CONFIG_H
#define __BSP_CONFIG_H

#include "stm32f10x.h"

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

/* ========== PMS7003 DMA 配置 ========== */
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

/* ==================== USART3 - Debug / Other ==================== */
#define BSP_DEBUG_USART                 USART3
#define BSP_DEBUG_USART_CLK             RCC_APB1Periph_USART3
#define BSP_DEBUG_GPIO_CLK              RCC_APB2Periph_GPIOB
#define BSP_DEBUG_TX_PIN                GPIO_Pin_10
#define BSP_DEBUG_TX_PORT               GPIOB
#define BSP_DEBUG_RX_PIN                GPIO_Pin_11
#define BSP_DEBUG_RX_PORT               GPIOB
#define BSP_DEBUG_BAUDRATE              115200U

#endif

#endif

