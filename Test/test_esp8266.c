#include "test_esp8266.h"// Device header

void Test_ESP8266_SendCmd(void) {
    // 步骤1: 发送一个会产生长响应的命令（确保响应中包含"OK"）
    Serial_SendString(USART2, "AT+GMR\r\n");
    Delay_ms(500); // 等待完整响应进入缓冲区（AT+GMR响应通常较长）
    
    // 步骤2: 手动检查缓冲区是否包含"OK"（模拟干扰存在）
    cbuf_handle_t rx_cbuf = BSP_USARTX_GetRxCbuf(USART2);
    uint8_t peek_buf[256];
    size_t avail = circular_buf_size(rx_cbuf);
    if (avail > 0) {
        size_t len = (avail > 255) ? 255 : avail;
        circular_buf_peek(rx_cbuf, peek_buf, len);
        peek_buf[len] = '\0';
        if (strstr((char*)peek_buf, "OK")) {
            Serial_Printf(USART_DEBUG, "⚠️ 缓冲区已存在干扰字符串 'OK'\r\n");
        }
    }
    
	
    // 步骤3: 发送新命令（此时缓冲区已有旧"OK"）
    _Bool result = ESP8266_SendCmd("AT\r\n", "OK");
    
    if (result == 0) {
        Serial_Printf(USART_DEBUG, "✅ 通过：正确匹配新响应\r\n");
    } else {
        Serial_Printf(USART_DEBUG, "❌ 失败：被旧响应干扰\r\n");
    }
        Delay_ms(1000);
}





