/**
	************************************************************
	************************************************************
	************************************************************
	*	文件名： 	esp8266.c
	*
	*	作者： 		张继瑞
	*
	*	日期： 		2017-05-08
	*
	*	版本： 		V1.0
	*
	*	说明： 		ESP8266的简单驱动
	*
	*	修改记录：	
	************************************************************
	************************************************************
	************************************************************
**/



#include "onenet.h"


#define ESP8266_SNTP_CONFIG "AT+CIPSNTPCFG=1,8,\"cn.ntp.org.cn\",\"ntp.sjtu.edu.cn\"\r\n"
#define ESP8266_SNTP_TIME "AT+CIPSNTPCFG?\r\n"

unsigned char esp8266_buf[512];
unsigned short esp8266_cnt = 0, esp8266_cntPre = 0;

//==========================================================
//	函数名称：	ESP8266_Clear
//
//	函数功能：	清空缓存
//
//	入口参数：	无
//
//	返回参数：	无
//
//	说明：		
//==========================================================
void ESP8266_Clear(void)
{

	memset(esp8266_buf, 0, sizeof(esp8266_buf));
	esp8266_cnt = 0;

}

//==========================================================
//	函数名称：	ESP8266_WaitRecive
//
//	函数功能：	等待接收完成
//
//	入口参数：	无
//
//	返回参数：	REV_OK-接收完成		REV_WAIT-接收超时未完成
//
//	说明：		循环调用检测是否接收完成
//==========================================================
_Bool ESP8266_WaitRecive(void)
{

	if(esp8266_cnt == 0) 							//如果接收计数为0 则说明没有处于接收数据中，所以直接跳出，结束函数
		return REV_WAIT;
		
	if(esp8266_cnt == esp8266_cntPre)				//如果上一次的值和这次相同，则说明接收完毕
	{
		esp8266_cnt = 0;							//清0接收计数
			
		return REV_OK;								//返回接收完成标志
	}
		
	esp8266_cntPre = esp8266_cnt;					//置为相同
	
	return REV_WAIT;								//返回接收未完成标志

}

//==========================================================
//	函数名称：	ESP8266_SendCmd
//
//	函数功能：	发送命令
//
//	入口参数：	cmd：命令
//				res：需要检查的返回指令
//
//	返回参数：	0-成功	1-失败
//
//	说明：		
//==========================================================
_Bool ESP8266_SendCmd(const char *cmd, char *res)
{
	
	unsigned char timeOut = 200;

	Usart_SendString(USART2, (unsigned char *)cmd, strlen((const char *)cmd));
	
	while(timeOut--)
	{
		if(ESP8266_WaitRecive() == REV_OK)							//如果收到数据
		{
			if(strstr((const char *)esp8266_buf, res) != NULL)		//如果检索到关键词
			{
				ESP8266_Clear();									//清空缓存
				
				return 0;
			}
		}
		
		Delay_ms(10);
	}
	
	return 1;

}

/**
  * @brief  通过ESP8266发送数据到服务器
  * @param  data 待发送数据指针
  * @param  len  期望发送的字节数
  * @return 实际发送成功的字节数（若失败返回0）
  */
int ESP8266_SendData(unsigned char *data, unsigned short len)
{
    char cmdBuf[32];
    int ret = 0;

    ESP8266_Clear(); // 清空接收缓存
    
    // 1. 发送AT命令设置数据长度
    sprintf(cmdBuf, "AT+CIPSEND=%d\r\n", len);
    if (!ESP8266_SendCmd(cmdBuf, ">")) // 等待">"提示符
    {
        // 2. 发送实际数据
        Usart_SendString(USART2, data, len); 
        
        // 3. 验证数据是否全部发送（根据实际需求实现）
        // 假设Usart_SendString返回实际发送的字节数
        // 若无法获取，可暂时返回len（需根据硬件特性调整）
        ret = len; 
        
        // 可选：等待发送完成（根据硬件特性添加延时或状态检查）
        // while (USART_GetFlagStatus(USART2, USART_FLAG_TC) == RESET);
    }
    else
    {
        // AT命令失败（如未收到">"）
        ret = 0;
    }

    return ret;
}

//==========================================================
//	函数名称：	ESP8266_GetIPD
//
//	函数功能：	获取平台返回的数据
//
//	入口参数：	等待的时间(乘以10ms)
//
//	返回参数：	平台返回的原始数据
//
//	说明：		不同网络设备返回的格式不同，需要去调试
//				如ESP8266的返回格式为	"+IPD,x:yyy"	x代表数据长度，yyy是数据内容
//==========================================================
unsigned char *ESP8266_GetIPD(unsigned short timeOut)
{

	char *ptrIPD = NULL;
	
	do
	{
		if(ESP8266_WaitRecive() == REV_OK)								//如果接收完成
		{
			ptrIPD = strstr((char *)esp8266_buf, "IPD,");				//搜索“IPD”头
			if(ptrIPD == NULL)											//如果没找到，可能是IPD头的延迟，还是需要等待一会，但不会超过设定的时间
			{
				//UsartPrintf(USART_DEBUG, "\"IPD\" not found\r\n");
			}
			else
			{
				ptrIPD = strchr(ptrIPD, ':');							//找到':'
				if(ptrIPD != NULL)
				{
					ptrIPD++;
					return (unsigned char *)(ptrIPD);
				}
				else
					return NULL;
				
			}
		}
		
		Delay_ms(5);													//延时等待
	} while(timeOut--);
	
	return NULL;														//超时还未找到，返回空指针

}
//////////////////////////////////////////////////////////////////////////////////////////


// 星期缩写映射表
//static const char* wday_map[] = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
//// 月份缩写映射表
//static const char* month_map[] = {"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};

//_Bool ESP8266_SNTP_Time(char *cmd, char *res, MYRTC* TimeStructure) {
//    char *response = NULL;
//    char timeStr[64] = {0};
//    unsigned char timeOut = 200;
//	

//    // 发送AT指令
//	ESP8266_Clear();
//    Usart_SendString(USART2, (unsigned char *)cmd, strlen(cmd));
//    
//    // 等待响应
//    while (timeOut--) {
//        if (ESP8266_WaitRecive() == REV_OK) {

//            response = (char*)esp8266_buf;
//            // 检查是否包含目标响应
//            if (strstr(response, res) != NULL) {
//			
//                // 提取时间字符串（示例响应格式：+CIPSNTPTIME:Sun Apr 20 13:35:18 2025）
//                char *timeStart = strstr(response, "+CIPSNTPTIME:");
//				if(timeStart == NULL){
//			OLED_ClearArea(66,46,50,16);
//			OLED_ShowString(66,46,"HERE44",OLED_7X12_HALF);
//			OLED_Update();
//			Delay_ms(500);				
//				}
//                if (timeStart) {

////                    sscanf(timeStart, "+CIPSNTPTIME:%3s %3s %hhu %hhu:%hhu:%hhu %hu",                                       
////                           timeStr, timeStr+4, &TimeStructure->Day,                 
////                           &TimeStructure->Hour, &TimeStructure->Minute,    
////                           &TimeStructure->Second, &TimeStructure->Year);
//                    
//                    // 解析星期（Sun -> 0, Mon -> 1...）
//                    for (int i = 0; i < 7; i++) {
//                        if (strncmp(timeStr, wday_map[i], 3) == 0) {
//                            TimeStructure->wday = i;
//                            break;
//                        }
//                    }
//                    
//                    // 解析月份（Apr -> 4）
//                    for (int i = 0; i < 12; i++) {
//                        if (strncmp(timeStr+4, month_map[i], 3) == 0) {
//                            TimeStructure->Month = i + 1;
//                            break;
//                        }
//                    }
//                    
//                    ESP8266_Clear();
//                    return 0; // 解析成功
//                }
//            }
//        }
//        Delay_ms(10);
//    }
//    return 1; // 超时或解析失败
//}


//////////////////////////////////////////////////////////////////////////////////////////
/**
 * @brief  ESP8266模块初始化
 * @return 初始化状态：
 *         0 - 成功
 *         1 - AT基础指令失败
 *         2 - 模式设置失败
 *         3 - DHCP设置失败
 *         4 - WiFi连接失败
 *         5 - 设置时区和 SNTP 服务器失败
 */
u8 ESP8266_Init(void)
{
    const u8 maxRetries = 3;    // 每个步骤最大重试次数
    u8 retryCount = 0;

    ESP8266_Clear();

    /* 1. 基础AT指令测试 */
    for (retryCount=0; retryCount<maxRetries; retryCount++) {
        if (ESP8266_SendCmd("AT\r\n", "OK") == 0) {
            break;
        }
        Delay_ms(500);
    }
    if (retryCount >= maxRetries) {
//        UsartPrintf(USART_DEBUG, "[ESP8266] AT cmd failed after %d tries\r\n", maxRetries);
                OLED_ClearArea(66,32,50,16);
                OLED_ShowString(66,32,"AT ERR",OLED_7X12_HALF);
        return 1;
    }

    /* 2. 设置STA模式 */
    for (retryCount=0; retryCount<maxRetries; retryCount++) {
        if (ESP8266_SendCmd("AT+CWMODE=1\r\n", "OK") == 0) {
            break;
        }
        Delay_ms(500);
    }
    if (retryCount >= maxRetries) {
//        UsartPrintf(USART_DEBUG, "[ESP8266] Set mode failed\r\n");
                OLED_ClearArea(66,32,50,16);
                OLED_ShowString(66,32,"STA ERR",OLED_7X12_HALF);
        return 2;
    }

    /* 3. 启用DHCP *///自动获取IP地址
    for (retryCount=0; retryCount<maxRetries; retryCount++) {
        if (ESP8266_SendCmd("AT+CWDHCP=1,1\r\n", "OK") == 0) {
            break;
        }
        Delay_ms(500);
    }
    if (retryCount >= maxRetries) {
//        UsartPrintf(USART_DEBUG, "[ESP8266] DHCP config failed\r\n");
                OLED_ClearArea(66,32,50,16);
                OLED_ShowString(66,32,"DHCP ERR",OLED_7X12_HALF);
        return 3;
    }

    /* 4. 连接WiFi */
    for (retryCount=0; retryCount<maxRetries; retryCount++) {
        if (ESP8266_SendCmd(WIFI_CONNECT_CMD, "GOT IP") == 0) {
            break;
        }
        Delay_ms(500);
    }
    if (retryCount >= maxRetries) {
//        UsartPrintf(USART_DEBUG, "[ESP8266] WiFi connect failed\r\n");
                OLED_ClearArea(66,32,50,16);
                OLED_ShowString(66,32,"WIFI ERR",OLED_7X12_HALF);
        return 4;
    }

//    /* 5. 设置时区和 SNTP 服务器 */
//    for (retryCount=0; retryCount<maxRetries; retryCount++) {
//        if (ESP8266_SendCmd(ESP8266_SNTP_CONFIG, "OK") == 0) {
//            break;
//        }
//        Delay_ms(500);
//    }
//    if (retryCount >= maxRetries) {
////        UsartPrintf(USART_DEBUG, "SNTP FAILED\r\n");
//              OLED_ClearArea(66,46,50,16);
//              OLED_ShowString(66,46,"SNTP FAILED",OLED_7X12_HALF);
//        return 5;
//    }

    /* 6. 设置时间 */
//    for (retryCount=0; retryCount<maxRetries; retryCount++) {
//        if (ESP8266_SNTP_Time("AT+CIPSNTPCFG?\r\n", "OK",&Time) == 0) {
//            break;
//        }
//        Delay_ms(500);
//    }
//    if (retryCount >= maxRetries) {
////        UsartPrintf(USART_DEBUG, "SET TIME FAILED");
//              OLED_ClearArea(66,46,50,16);
//              OLED_ShowString(66,46,"TIME FAILED",OLED_7X12_HALF);
//        return 6;
//    }
//    UsartPrintf(USART_DEBUG, "[ESP8266] Init success\r\n");
        OLED_ClearArea(66,32,50,16);//这句是用于清除前面输出的失败信息，既已执行到这，说明连接成功，所以要清除
    return 0;
}


//==========================================================
//	函数名称：	USART2_IRQHandler
//
//	函数功能：	串口2收发中断
//
//	入口参数：	无
//
//	返回参数：	无
//
//	说明：		
//==========================================================
void USART2_IRQHandler(void)
{
	
	if(USART_GetITStatus(USART2, USART_IT_RXNE) != RESET) //接收中断
	{
		if(esp8266_cnt >= sizeof(esp8266_buf))	esp8266_cnt = 0; //防止串口被刷爆
		esp8266_buf[esp8266_cnt++] = USART2->DR;
		
		USART_ClearFlag(USART2, USART_FLAG_RXNE);
	}

}
