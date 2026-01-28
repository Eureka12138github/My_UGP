/**
 ************************************************************
 * 文件名：    esp8266.c
 * 作者：      张继瑞（重构：Eureka）
 * 日期：      2017-05-08（重构于 2026）
 * 说明：      ESP8266 驱动（基于环形缓冲区串口）
 ************************************************************
 */

#include "onenet.h"         // 确保包含  bsp_usart_plus.h
#include <string.h>
#include <stdio.h>
#include "bsp_usart.h"


#define ESP8266_RX_BUF_SIZE 512
static uint8_t s_rx_buffer[ESP8266_RX_BUF_SIZE];  // 局部接收缓存


static size_t ESP8266_FetchResponse(void)
{
    size_t total = 0;

    // 先读当前所有可用数据
    while (Serial_Available(USART2) && total < ESP8266_RX_BUF_SIZE - 1) {
        s_rx_buffer[total++] = Serial_ReadByte(USART2);
    }

    // 关键：再等待一小段时间（比如 10ms），看是否有“后续字节”（应对分包）
    for (int i = 0; i < 10; i++) {  // 最多等 10ms
        Delay_ms(1);
        while (Serial_Available(USART2) && total < ESP8266_RX_BUF_SIZE - 1) {
            s_rx_buffer[total++] = Serial_ReadByte(USART2);
        }
        // 如果某次 Delay 后没新数据，可以提前退出（可选优化）
        // if (!Serial_Available(USART2)) break;
    }

    s_rx_buffer[total] = '\0';  // 确保字符串结尾
    return total;
}

//==========================================================
// 函数名称：   ESP8266_SendCmd
// 功能：       发送 AT 命令并等待指定响应
// 入口参数：   cmd - AT 命令字符串（含 \r\n）
//              res - 期望在响应中出现的子串（如 "OK", "GOT IP"）
// 返回：       0 - 成功；1 - 超时或未找到响应
//==========================================================
_Bool ESP8266_SendCmd(const char *cmd, const char *res)
{
    // 清空 USART2 接收缓冲区（丢弃历史数据）
	//
    while (Serial_Available(USART2)) {
        (void)Serial_ReadByte(USART2);
    }

    // 发送命令
    Serial_SendString(USART2, cmd);

    // 等待响应（最多 2 秒）
    for (uint8_t timeOut = 200; timeOut > 0; timeOut--) {
        Delay_ms(10);

        size_t len = ESP8266_FetchResponse();
        if (len > 0) {
            s_rx_buffer[len] = '\0'; // 确保字符串结尾

            if (strstr((char*)s_rx_buffer, res) != NULL) {
                return 0; // 找到响应，成功
            }
        }
    }

    return 1; // 超时失败
}

//==========================================================
// 函数名称：   ESP8266_SendData
// 功能：       通过 ESP8266 发送数据到服务器
// 入口参数：   data - 数据指针；len - 数据长度
// 返回：       实际发送字节数（成功为 len，失败为 0）
//==========================================================
int ESP8266_SendData(unsigned char *data, unsigned short len)
{
    char cmdBuf[32];
    
    // 清空历史数据（重要！）
    while (Serial_Available(USART2)) {
        (void)Serial_ReadByte(USART2);
    }

    snprintf(cmdBuf, sizeof(cmdBuf), "AT+CIPSEND=%d\r\n", len);
    if (ESP8266_SendCmd(cmdBuf, ">") != 0) {
        return 0;
    }

    Serial_SendArray(USART2, data, len);

    // 【新增】等待并丢弃 "SEND OK" 响应
    Delay_ms(100); // 等待 ESP8266 返回
    while (Serial_Available(USART2)) {
        (void)Serial_ReadByte(USART2); // 丢弃 SEND OK 等响应
    }

    return len;
}


	/*
		逻辑梳理：
					目的是当平台有数据下发时，按照指定格式索引，如果有合法数据，就返回指向数据内容的指针，其他函数知道具体数据的指针后，
					可以进行解析操作。本函数不会返回具体的下发数据！
					
					先清理一下缓冲区 s_rx_buffer 防止还有其他残留数据
					调用 ESP8266_FetchResponse 抓拍一下当前串口对应接收环形缓冲区数据快照，从 0 开始压入 s_rx_buffer 缓冲区
					对此缓冲区进行检索，如果检索成功，即返回一个指向这个缓冲区 s_rx_buffer 有效数据的一个指针，外部函数通过
					指针进行数据的读取访问，解析；
					如果没有数据，就返回NULL，表示暂时不需要进行数据解析
	
	
					但现在我看出的问题有：
					1、ESP8266_FetchResponse 所抓取的快照可能数据不全，即虽然有前面的+IPD等有效信息，但是还没读取到
					数据包的末尾，即数据被截断了？！
	
					2、ESP8266_FetchResponse 的实现也有问题，现在是没调取一次，如果有数据，就往 s_rx_buffer 里面填充
					而且是从 0 开始！！！这问题很大，因为一个有效数据包不一定能够在一次快照内完全包含！
					完全有可能第一次快照只截了个开头，然后后面数据没了，而在 ESP8266_GetIPD 中只检测数据的开头，所以判断为有效
					实则数据包被截断了都不知道！而且下次快照的时候，数据包未截取到的部分会重新覆盖 s_rx_buffer 开头，从而把数据包“肢解”了！
					
					我想到的方法是，要么 ESP8266_FetchResponse 中把数据收全了 再执行 ESP8266_GetIPD 检索数据包是否合法
					要么，在 ESP8266_GetIPD 中检测数据包的完整性后，再返回开头正确的指针，如果是残躯的数据包，等完整后再返回。
					
	
	
	*/


//==========================================================
// 函数名称：   ESP8266_GetIPD
// 功能：       获取服务器下发的数据（+IPD,x:... 格式）
// 入口参数：   timeOut - 超时时间（单位：10ms）
// 返回：       指向数据内容的指针（如 "+IPD,12:hello" → 返回 "hello"）
//              若超时或格式错误，返回 NULL
//==========================================================
unsigned char *ESP8266_GetIPD(unsigned short timeOut)
{

    do {
        Delay_ms(5);
		//由于环形缓冲区数据没清，大概率len > 0
        size_t len = ESP8266_FetchResponse();
        if (len > 0) {
			Serial_Printf(USART_DEBUG, "AAAAAAAAAAAAAAAAAAAA\r\n");
			Serial_Printf(USART_DEBUG, "Recv: %s\r\n", s_rx_buffer);
			// 替换原来的 Serial_Printf
Serial_Printf(USART_DEBUG, "Recv HEX: %02X %02X %02X %02X ...\r\n", 
    s_rx_buffer[0], s_rx_buffer[1], s_rx_buffer[2], s_rx_buffer[3]);
			Serial_Printf(USART_DEBUG, "XXXXXXXXXXXXXXXXXXXX\r\n");
			Delay_ms(500);
            // 注意：Serial_ReadArray 已经写了 len 字节，我们只需确保字符串安全
            s_rx_buffer[len] = '\0'; // 防止越界（虽然 FetchResponse 限制了长度）
			//这里可否输出接收到的信息看看？
            char *ptrIPD = strstr((char*)s_rx_buffer, "+IPD,");
            if (ptrIPD) {
                ptrIPD = strchr(ptrIPD, ':');
                if (ptrIPD && *(ptrIPD + 1) != '\0') {
                    return (unsigned char*)(ptrIPD + 1);
                }
            }
        }
    } while (timeOut-- > 0);

    return NULL;
}
//==========================================================
// 函数名称：   ESP8266_Init
// 功能：       初始化 ESP8266 模块（STA 模式 + 连接 WiFi）
// 返回：       0 - 成功；非 0 - 错误码
//==========================================================
u8 ESP8266_Init(void)
{
    const u8 maxRetries = 3;
    u8 retryCount;

    /* 1. 测试 AT */
    for (retryCount = 0; retryCount < maxRetries; retryCount++) {
        if (ESP8266_SendCmd("AT\r\n", "OK") == 0) break;
        Delay_ms(500);
    }
    if (retryCount >= maxRetries) {
        OLED_ClearArea(66, 32, 50, 16);
        OLED_ShowString(66, 32, "AT ERR", OLED_7X12_HALF);
        return 1;
    }

    /* 2. 设置 STA 模式 */
    for (retryCount = 0; retryCount < maxRetries; retryCount++) {
        if (ESP8266_SendCmd("AT+CWMODE=1\r\n", "OK") == 0) break;
        Delay_ms(500);
    }
    if (retryCount >= maxRetries) {
        OLED_ClearArea(66, 32, 50, 16);
        OLED_ShowString(66, 32, "STA ERR", OLED_7X12_HALF);
        return 2;
    }

    /* 3. 启用 DHCP */
    for (retryCount = 0; retryCount < maxRetries; retryCount++) {
        if (ESP8266_SendCmd("AT+CWDHCP=1,1\r\n", "OK") == 0) break;
        Delay_ms(500);
    }
    if (retryCount >= maxRetries) {
        OLED_ClearArea(66, 32, 50, 16);
        OLED_ShowString(66, 32, "DHCP ERR", OLED_7X12_HALF);
        return 3;
    }

    /* 4. 连接 WiFi */
    for (retryCount = 0; retryCount < maxRetries; retryCount++) {
        if (ESP8266_SendCmd(WIFI_CONNECT_CMD, "GOT IP") == 0) break;
        Delay_ms(500);
    }
    if (retryCount >= maxRetries) {
        OLED_ClearArea(66, 32, 50, 16);
        OLED_ShowString(66, 32, "WIFI ERR", OLED_7X12_HALF);
        return 4;
    }

    OLED_ClearArea(66, 32, 50, 16); // 清除错误提示
    return 0;
}


