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

#endif