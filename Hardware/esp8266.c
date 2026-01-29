/**
 ************************************************************
 * 文件名：    esp8266.c
 * 作者：      Eureka
 * 日期：      2017-05-08（重构于 2026）
 * 说明：      ESP8266 驱动（基于环形缓冲区串口，无全局接收缓存）
 ************************************************************
 */

#include "onenet.h"         // 确保包含 bsp_usart.h
#include <string.h>
#include <stdio.h>
#include "bsp_usart.h"
#include "Delay.h"
#include "esp8266_drv.h"


//==========================================================
// 函数名称：   ESP8266_SendData
// 功能：       发送数据到 TCP 连接
// 入口参数：   data - 数据指针，len - 长度
// 返回：       true - 成功；false - 失败
//==========================================================
_Bool ESP8266_SendData(unsigned char *data, unsigned short len)
{
    char cmdBuf[32];
    snprintf(cmdBuf, sizeof(cmdBuf), "AT+CIPSEND=%d\r\n", len);
    
    if (ESP8266_SendCmd(cmdBuf, ">") != 0) {
        return false;
    }

    Serial_SendArray(USART2, data, len);

	//下面这段为响应判断逻辑，应该优化！用状态机的方式，不要在这等响应阻塞主循环
    cbuf_handle_t rx_cbuf = BSP_USARTX_GetRxCbuf(USART2);
    if (!rx_cbuf) return false;
	
    uint32_t start = SysTick_Get();
    while ((SysTick_Get() - start) < 1000) { // 1秒超时
        size_t avail = circular_buf_size(rx_cbuf);
        if (avail > 0) {
            uint8_t temp[64];
            size_t len = (avail > sizeof(temp) - 1) ? sizeof(temp) - 1 : avail;
            circular_buf_peek(rx_cbuf, temp, len);
            temp[len] = '\0';

            if (strstr((char*)temp, "SEND OK")) {
                return true;
            }
            if (strstr((char*)temp, "SEND FAIL")) {
                break;
            }
        }
        Delay_ms(10);
    }
    return false;
}

//==========================================================
// 函数名称：   ESP8266_GetIPD
// 功能：       从 USART2 环形缓冲区中解析 +IPD,x:data
// 入口参数：   timeOut_ms - 超时时间（毫秒）
// 返回：       指向 data 内容的指针（静态缓存，下次调用失效）
//              NULL 表示超时或格式错误
//==========================================================
unsigned char *ESP8266_GetIPD(uint16_t timeOut_ms)
{
    static uint8_t s_parse_buf[512]; // 仅用于协议解析快照
    cbuf_handle_t rx_cbuf = BSP_USARTX_GetRxCbuf(USART2);
    if (!rx_cbuf) return NULL;

    uint32_t start = SysTick_Get();
    while ((SysTick_Get() - start) < timeOut_ms) {
        size_t avail = circular_buf_size(rx_cbuf);
        if (avail == 0) {
            Delay_ms(2);
            continue;
        }

        size_t peek_len = (avail > sizeof(s_parse_buf) - 1) ? sizeof(s_parse_buf) - 1 : avail;
        circular_buf_peek(rx_cbuf, s_parse_buf, peek_len);
        s_parse_buf[peek_len] = '\0';

        char *ipd = strstr((char*)s_parse_buf, "+IPD,");
        if (ipd) {
            char *colon = strchr(ipd, ':');
            if (colon) {
                int data_len = 0;
                if (sscanf(ipd + 5, "%d", &data_len) == 1 && data_len > 0) {
                    size_t data_offset = colon - (char*)s_parse_buf + 1;
                    if (data_offset + data_len <= peek_len) {
                        // ✅ 完整包已到！

                        // 可选：跳过已处理的数据（避免重复解析）
                        // circular_buf_skip(rx_cbuf, data_offset + data_len);

                        return (unsigned char*)(s_parse_buf + data_offset);
                    }
                }
            }
        }
        Delay_ms(5);
    }
    return NULL;
}


