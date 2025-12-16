/**
	************************************************************
	************************************************************
	************************************************************
	*	文件名： 	onenet.c
	*
	*	作者： 		张继瑞
	*
	*	日期： 		2017-05-08
	*
	*	版本： 		V1.1
	*
	*	说明： 		与onenet平台的数据交互接口层
	*
	*	修改记录：	V1.0：协议封装、返回判断都在同一个文件，并且不同协议接口不同。
	*				V1.1：提供统一接口供应用层使用，根据不同协议文件来封装协议相关的内容。
	************************************************************
	************************************************************
	************************************************************
**/


#include "onenet.h"



char devid[16];

char key[48];


extern unsigned char esp8266_buf[512];


/*
************************************************************
*	函数名称：	OTA_UrlEncode
*
*	函数功能：	sign需要进行URL编码
*
*	入口参数：	sign：加密结果
*
*	返回参数：	0-成功	其他-失败
*
*	说明：		+			%2B
*				空格		%20
*				/			%2F
*				?			%3F
*				%			%25
*				#			%23
*				&			%26
*				=			%3D
************************************************************
*/
static unsigned char OTA_UrlEncode(char *sign)
{

	char sign_t[40];
	unsigned char i = 0, j = 0;
	unsigned char sign_len = strlen(sign);
	
	if(sign == (void *)0 || sign_len < 28)
		return 1;
	
	for(; i < sign_len; i++)
	{
		sign_t[i] = sign[i];
		sign[i] = 0;
	}
	sign_t[i] = 0;
	
	for(i = 0, j = 0; i < sign_len; i++)
	{
		switch(sign_t[i])
		{
			case '+':
				strcat(sign + j, "%2B");j += 3;
			break;
			
			case ' ':
				strcat(sign + j, "%20");j += 3;
			break;
			
			case '/':
				strcat(sign + j, "%2F");j += 3;
			break;
			
			case '?':
				strcat(sign + j, "%3F");j += 3;
			break;
			
			case '%':
				strcat(sign + j, "%25");j += 3;
			break;
			
			case '#':
				strcat(sign + j, "%23");j += 3;
			break;
			
			case '&':
				strcat(sign + j, "%26");j += 3;
			break;
			
			case '=':
				strcat(sign + j, "%3D");j += 3;
			break;
			
			default:
				sign[j] = sign_t[i];j++;
			break;
		}
	}
	
	sign[j] = 0;
	
	return 0;

}

/*
************************************************************
*	函数名称：	OTA_Authorization
*
*	函数功能：	计算Authorization
*
*	入口参数：	ver：参数组版本号，日期格式，目前仅支持格式"2018-10-31"
*				res：产品id
*				et：过期时间，UTC秒值
*				access_key：访问密钥
*				dev_name：设备名
*				authorization_buf：缓存token的指针
*				authorization_buf_len：缓存区长度(字节)
*
*	返回参数：	0-成功	其他-失败
*
*	说明：		当前仅支持sha1
************************************************************
*/
#define METHOD		"sha1"
static unsigned char OneNET_Authorization(const char *ver,
                                          const char *res,
                                          unsigned int et,
                                          const char *access_key,
                                          const char *dev_name,
                                          char *authorization_buf,
                                          unsigned short authorization_buf_len,
                                          _Bool flag)
{
	
	size_t olen = 0;
	
	char sign_buf[64];								//保存签名的Base64编码结果 和 URL编码结果
	char hmac_sha1_buf[64];							//保存签名
	char access_key_base64[64];						//保存access_key的Base64编码结合
	char string_for_signature[72];					//保存string_for_signature，这个是加密的key

//----------------------------------------------------参数合法性--------------------------------------------------------------------
	if(ver == (void *)0 || res == (void *)0 || et < 1564562581 || access_key == (void *)0
		|| authorization_buf == (void *)0 || authorization_buf_len < 120)
		return 1;
	
//----------------------------------------------------将access_key进行Base64解码----------------------------------------------------
	memset(access_key_base64, 0, sizeof(access_key_base64));
	BASE64_Decode((unsigned char *)access_key_base64, sizeof(access_key_base64), &olen, (unsigned char *)access_key, strlen(access_key));
//	UsartPrintf(USART_DEBUG, "access_key_base64: %s\r\n", access_key_base64);
	
//----------------------------------------------------计算string_for_signature-----------------------------------------------------
	memset(string_for_signature, 0, sizeof(string_for_signature));
	if(flag)
		snprintf(string_for_signature, sizeof(string_for_signature), "%d\n%s\nproducts/%s\n%s", et, METHOD, res, ver);
	else
		snprintf(string_for_signature, sizeof(string_for_signature), "%d\n%s\nproducts/%s/devices/%s\n%s", et, METHOD, res, dev_name, ver);
//	UsartPrintf(USART_DEBUG, "string_for_signature: %s\r\n", string_for_signature);
	
//----------------------------------------------------加密-------------------------------------------------------------------------
	memset(hmac_sha1_buf, 0, sizeof(hmac_sha1_buf));
	
	hmac_sha1((unsigned char *)access_key_base64, strlen(access_key_base64),
				(unsigned char *)string_for_signature, strlen(string_for_signature),
				(unsigned char *)hmac_sha1_buf);
	
//	UsartPrintf(USART_DEBUG, "hmac_sha1_buf: %s\r\n", hmac_sha1_buf);
	
//----------------------------------------------------将加密结果进行Base64编码------------------------------------------------------
	olen = 0;
	memset(sign_buf, 0, sizeof(sign_buf));
	BASE64_Encode((unsigned char *)sign_buf, sizeof(sign_buf), &olen, (unsigned char *)hmac_sha1_buf, strlen(hmac_sha1_buf));

//----------------------------------------------------将Base64编码结果进行URL编码---------------------------------------------------
	OTA_UrlEncode(sign_buf);
//	UsartPrintf(USART_DEBUG, "sign_buf: %s\r\n", sign_buf);
	
//----------------------------------------------------计算Token--------------------------------------------------------------------
	if(flag)
		snprintf(authorization_buf, authorization_buf_len, "version=%s&res=products%%2F%s&et=%d&method=%s&sign=%s", ver, res, et, METHOD, sign_buf);
	else
		snprintf(authorization_buf, authorization_buf_len, "version=%s&res=products%%2F%s%%2Fdevices%%2F%s&et=%d&method=%s&sign=%s", ver, res, dev_name, et, METHOD, sign_buf);
//	UsartPrintf(USART_DEBUG, "Token: %s\r\n", authorization_buf);
	
	return 0;

}

//==========================================================
//	函数名称：	OneNET_RegisterDevice
//
//	函数功能：	在产品中注册一个设备
//
//	入口参数：	access_key：访问密钥
//				pro_id：产品ID
//				serial：唯一设备号
//				devid：保存返回的devid
//				key：保存返回的key
//
//	返回参数：	0-成功		1-失败
//
//	说明：		
//==========================================================
_Bool OneNET_RegisterDevice(void)
{

	_Bool result = 1;
	unsigned short send_len = 11 + strlen(ONENET_DEVICE_NAME);
	char *send_ptr = NULL, *data_ptr = NULL;
	
	char authorization_buf[144];													//加密的key
	
	send_ptr = malloc(send_len + 240);
	if(send_ptr == NULL)
		return result;
	
	while(ESP8266_SendCmd("AT+CIPSTART=\"TCP\",\"183.230.40.33\",80\r\n", "CONNECT"))
		Delay_ms(500);
	
	OneNET_Authorization("2018-10-31", ONENET_PROID, 1956499200, ONENET_ACCESS_KEY, NULL,
							authorization_buf, sizeof(authorization_buf), 1);
	
	snprintf(send_ptr, 240 + send_len, "POST /mqtt/v1/devices/reg HTTP/1.1\r\n"
					"Authorization:%s\r\n"
					"Host:ota.heclouds.com\r\n"
					"Content-Type:application/json\r\n"
					"Content-Length:%d\r\n\r\n"
					"{\"name\":\"%s\"}",
	
					authorization_buf, 11 + strlen(ONENET_DEVICE_NAME), ONENET_DEVICE_NAME);
	
	ESP8266_SendData((unsigned char *)send_ptr, strlen(send_ptr));
	
	/*
	{
	  "request_id" : "f55a5a37-36e4-43a6-905c-cc8f958437b0",
	  "code" : "onenet_common_success",
	  "code_no" : "000000",
	  "message" : null,
	  "data" : {
		"device_id" : "589804481",
		"name" : "mcu_id_43057127",
		
	"pid" : 282932,
		"key" : "indu/peTFlsgQGL060Gp7GhJOn9DnuRecadrybv9/XY="
	  }
	}
	*/
	
	data_ptr = (char *)ESP8266_GetIPD(250);							//等待平台响应
	
	if(data_ptr)
	{
		data_ptr = strstr(data_ptr, "device_id");
	}
	
	if(data_ptr)
	{
		char name[16];
		int pid = 0;
		
		if(sscanf(data_ptr, "device_id\" : \"%[^\"]\",\r\n\"name\" : \"%[^\"]\",\r\n\r\n\"pid\" : %d,\r\n\"key\" : \"%[^\"]\"", devid, name, &pid, key) == 4)
		{
//			UsartPrintf(USART_DEBUG, "create device: %s, %s, %d, %s\r\n", devid, name, pid, key);
			result = 0;
		}
	}
	
	free(send_ptr);
	ESP8266_SendCmd("AT+CIPCLOSE\r\n", "OK");
	
	return result;

}

//==========================================================
//	函数名称：	OneNet_DevLink
//
//	函数功能：	与onenet创建连接
//
//	入口参数：	无
//
//	返回参数：	0-成功	
//              1-鉴权失败	
//              2-MQTT包构造失败	
//              3-等待响应超时	
//              4-非CONNACK包	
//              5-CONNACK数据包格式错误  // 依据: MQTT_UnPacketConnectAck返回1(长度字段错误)或255(标志位字段错误)
//              6-协议版本不可接受      // 依据: MQTT协议CONNACK返回码1
//              7-客户端标识符被拒绝    // 依据: MQTT协议CONNACK返回码2
//              8-服务端不可用          // 依据: MQTT协议CONNACK返回码3
//              9-用户名或密码错误      // 依据: MQTT协议CONNACK返回码4
//              10-未授权连接           // 依据: MQTT协议CONNACK返回码5
//              11-未知连接错误
//
//	说明：		与onenet平台建立连接
//==========================================================
unsigned char OneNet_DevLink(void)
{
	MQTT_PACKET_STRUCTURE mqttPacket = {NULL, 0, 0, 0};	//协议包
	unsigned char *dataPtr;
	char authorization_buf[160];
	unsigned char result = 1; // 默认为鉴权失败
	
	// 根据设备ID、设备秘钥、设备名称生成OneNET平台鉴权Token
	if (OneNET_Authorization("2018-10-31", ONENET_PROID, 1956499200, ONENET_ACCESS_KEY, ONENET_DEVICE_NAME,
		authorization_buf, sizeof(authorization_buf), 0) != 0) {
		result = 1; // 鉴权Token生成失败
		goto exit;
	}
	
	// 构造MQTT CONNECT包
	if(MQTT_PacketConnect(ONENET_PROID, authorization_buf, ONENET_DEVICE_NAME, 256, 1, MQTT_QOS_LEVEL0, NULL, NULL, 0, &mqttPacket) != 0)
	{
		result = 2; // MQTT包构造失败
		goto exit;
	}
	
	// 发送数据到平台
	ESP8266_SendData(mqttPacket._data, mqttPacket._len);
	
	// 等待平台响应
	dataPtr = ESP8266_GetIPD(250);
	if(dataPtr != NULL)
	{
		// 解析收到的数据包
		if(MQTT_UnPacketRecv(dataPtr) == MQTT_PKT_CONNACK)
		{
			// 检查连接确认结果
			unsigned char connack_code = MQTT_UnPacketConnectAck(dataPtr);
			
			switch(connack_code)
			{
				case 0:
					result = 0; // 连接成功
					break;
				case 1:
					result = 6; // 协议版本不可接受 (MQTT协议标准)
					break;
				case 2:
					result = 7; // 客户端标识符被拒绝 (MQTT协议标准)
					break;
				case 3:
					result = 8; // 服务端不可用 (MQTT协议标准)
					break;
				case 4:
					result = 9; // 用户名或密码错误 (MQTT协议标准)
					break;
				case 5:
					result = 10; // 未授权连接 (MQTT协议标准)
					break;
				case 255:
					result = 5; // 数据包格式错误 (MQTT_UnPacketConnectAck实现)
					break;
				default:
					result = 11; // 未知连接错误
					break;
			}
		}
		else
		{
			result = 4; // 接收到非CONNACK包
		}
	}
	else
	{
		result = 3; // 等待响应超时
	}
	
exit:
	// 释放MQTT包缓冲区
	MQTT_DeleteBuffer(&mqttPacket);
	
	return result;
}

extern u8 temp,humi;
extern u16 Dust_Limit;
extern u8 Noise_Limit;
extern u16 decibels;//当前环境中分贝大小
extern u16 PM2_5_ENV;
unsigned char OneNet_FillBuf(char *buf)
{
	
	char text[48];
	
	memset(text, 0, sizeof(text));
	
	strcpy(buf, "{\"id\":\"123\",\"params\":{");
	
	memset(text, 0, sizeof(text));
	sprintf(text, "\"temp\":{\"value\":%d},", temp);//温度
	strcat(buf, text);
	
	memset(text, 0, sizeof(text));
	sprintf(text, "\"humi\":{\"value\":%d},", humi);//湿度
	strcat(buf, text);
	
	memset(text, 0, sizeof(text));
	sprintf(text, "\"dust\":{\"value\":%d},", PM2_5_ENV);//扬尘
	strcat(buf, text);
	
	memset(text, 0, sizeof(text));
	sprintf(text, "\"noise\":{\"value\":%d},", decibels);//噪音
	strcat(buf, text);
	
	memset(text, 0, sizeof(text));
	sprintf(text, "\"noise_limit\":{\"value\":%d},", Noise_Limit);//噪音阈值
	strcat(buf, text);
	
	memset(text, 0, sizeof(text));
	sprintf(text, "\"dust_limit\":{\"value\":%d},", Dust_Limit);//扬尘阈值
	strcat(buf, text);
	

	memset(text, 0, sizeof(text));
	sprintf(text, "\"dust_excess\":{\"value\":%s},",(PM2_5_ENV > Dust_Limit) ? "true" : "false");//扬尘过大
	strcat(buf, text);
	
	memset(text, 0, sizeof(text));
	sprintf(text, "\"noise_excess\":{\"value\":%s},", (decibels > Noise_Limit) ? "true" : "false");//噪音过大
	strcat(buf, text);
	
	memset(text, 0, sizeof(text));
	sprintf(text, "\"led\":{\"value\":%s}", led_info.Led_Status ? "true" : "false");//LED
	strcat(buf, text);
	
	strcat(buf, "}}");
	
	return strlen(buf);

}


/**
 * @brief 发送数据到OneNET平台
 * @return 0-成功，其他值见错误码定义
 * 
 * 错误码定义：
 *  0 - 发送成功
 * -1 - 数据封包失败
 * -2 - 网络发送失败 
 * -3 - 无效数据长度
 * -4 - 缓冲区溢出
 */
int8_t OneNet_SendData(void)
{
    MQTT_PACKET_STRUCTURE mqttPacket = {NULL, 0, 0, 0};
    char payloadBuf[256];
    int8_t ret = -3; // 默认错误：无效数据长度
    int sentBytes = 0;
    uint16_t body_len = 0; // 统一使用无符号类型

    // 初始化阶段
    memset(payloadBuf, 0, sizeof(payloadBuf));
    
    /* 数据准备阶段 */
    body_len = OneNet_FillBuf(payloadBuf);
    if (body_len == 0 || body_len >= sizeof(payloadBuf)) {
//        UsartPrintf(USART_DEBUG, "[ERROR] Invalid payload len:%u\r\n", body_len);
        return -3;
    }

    /* 数据封包阶段 */
    if (MQTT_PacketSaveData(ONENET_PROID, ONENET_DEVICE_NAME, body_len, NULL, &mqttPacket) != 0) {
//        UsartPrintf(USART_DEBUG, "[ERROR] MQTT pack failed\r\n");
        return -1;
    }

    /* 数据填充阶段 */
    for (uint16_t i = 0; i < body_len; i++) {
        if (mqttPacket._len >= mqttPacket._size) {
//            UsartPrintf(USART_DEBUG, "[ERROR] Buffer overflow %u/%u\r\n",
//                       mqttPacket._len, mqttPacket._size);
            ret = -4;
            goto cleanup;
        }
        mqttPacket._data[mqttPacket._len++] = payloadBuf[i];
    }

    /* 数据发送阶段 */
    sentBytes = ESP8266_SendData(mqttPacket._data, mqttPacket._len);
    if (sentBytes != mqttPacket._len) {
//        UsartPrintf(USART_DEBUG, "[ERROR] Send %d/%d bytes\r\n", 
//                   sentBytes, mqttPacket._len);
        ret = -2;
        goto cleanup;
    }

//    UsartPrintf(USART_DEBUG, "[INFO] Send %u bytes success\r\n", mqttPacket._len);
    ret = 0;

cleanup:
    /* 资源清理阶段 */
    if (mqttPacket._data) {
        MQTT_DeleteBuffer(&mqttPacket);
    }
    return ret;
}

//==========================================================
//	函数名称：	OneNET_Publish
//
//	函数功能：	发布消息
//
//	入口参数：	topic：发布的主题
//				msg：消息内容
//
//	返回参数：	无
//
//	说明：		
//==========================================================
void OneNET_Publish(const char *topic, const char *msg)
{

	MQTT_PACKET_STRUCTURE mqtt_packet = {NULL, 0, 0, 0};						//协议包
	
//	UsartPrintf(USART_DEBUG, "Publish Topic: %s, Msg: %s\r\n", topic, msg);
	
	if(MQTT_PacketPublish(MQTT_PUBLISH_ID, topic, msg, strlen(msg), MQTT_QOS_LEVEL0, 0, 1, &mqtt_packet) == 0)
	{
		ESP8266_SendData(mqtt_packet._data, mqtt_packet._len);					//向平台发送订阅请求
		
		MQTT_DeleteBuffer(&mqtt_packet);										//删包
	}

}

//==========================================================
//	函数名称：	OneNET_Subscribe
//
//	函数功能：	订阅
//
//	入口参数：	无
//
//	返回参数：	无
//
//	说明：		
//==========================================================
void OneNET_Subscribe(void)
{
	
	MQTT_PACKET_STRUCTURE mqtt_packet = {NULL, 0, 0, 0};						//协议包
	
	char topic_buf[56];
	const char *topic = topic_buf;
	//"$sys/%s/%s/thing/property/set"
	snprintf(topic_buf, sizeof(topic_buf), "$sys/%s/%s/thing/property/set", ONENET_PROID, ONENET_DEVICE_NAME);
	
//	UsartPrintf(USART_DEBUG, "Subscribe Topic: %s\r\n", topic_buf);
	
	if(MQTT_PacketSubscribe(MQTT_SUBSCRIBE_ID, MQTT_QOS_LEVEL0, &topic, 1, &mqtt_packet) == 0)
	{
		ESP8266_SendData(mqtt_packet._data, mqtt_packet._len);					//向平台发送订阅请求
		
		MQTT_DeleteBuffer(&mqtt_packet);										//删包
	}

}

//==========================================================
//	函数名称：	OneNet_RevPro
//
//	函数功能：	平台返回数据检测
//
//	入口参数：	dataPtr：平台返回的数据
//
//	返回参数：	无
//
//	说明：		
//==========================================================
//extern bool LedMode;
u8 OneNet_RevPro(unsigned char *cmd)
{	
	char *req_payload = NULL;
	char *cmdid_topic = NULL;
//	char *id_str = NULL; //命令ID
	unsigned short topic_len = 0;
	unsigned short req_len = 0;
	unsigned char qos = 0;
	static unsigned short pkt_id = 0;	
	unsigned char type = 0;
	int result = 0; // 修改为int类型
	cJSON *raw_json, *params_json;
//	cJSON *led_json, *id_json;
	cJSON *dust_limit_json; // 解析扬尘阈值
	cJSON *noise_limit_json; // 解析噪音阈值
	
	type = MQTT_UnPacketRecv(cmd);
	switch(type)
	{
		case MQTT_PKT_PUBLISH: // 接收的Publish消息
		
			result = MQTT_UnPacketPublish(cmd, &cmdid_topic, &topic_len, &req_payload, &req_len, &qos, &pkt_id);
			if(result == 0)
			{
//				UsartPrintf(USART_DEBUG, "topic: %s, topic_len: %d, payload: %s, payload_len: %d\r\n",
//																	cmdid_topic, topic_len, req_payload, req_len);
				// 解析 JSON 并提取 id
				raw_json = cJSON_Parse(req_payload);
				if (raw_json == NULL) {
//					UsartPrintf(USART_DEBUG, "Error: JSON 解析失败\r\n");
					return 1; // 解析失败，返回1
				}

				// 提取 id
//				id_json = cJSON_GetObjectItem(raw_json, "id");
//				if (id_json != NULL && (id_json->type == cJSON_String)) {
//					id_str = id_json->valuestring;
////					UsartPrintf(USART_DEBUG, "ID: %s\r\n", id_str);
//				} else {
////					UsartPrintf(USART_DEBUG, "Error: 无效的 id 字段\r\n");
//					result = 1; // 无效的id字段，设置result为1
//				}
				
				// 提取 LED 状态
				params_json = cJSON_GetObjectItem(raw_json, "params");
				dust_limit_json = cJSON_GetObjectItem(params_json, "dust_limit");
				noise_limit_json = cJSON_GetObjectItem(params_json, "noise_limit");
//				led_json = cJSON_GetObjectItem(params_json, "led");
				
				if (dust_limit_json != NULL) {
					Dust_Limit = dust_limit_json->valueint; // 解析云端最新的阈值并更新本地阈值
					Store_Data[1] = Dust_Limit;
					Store_Save();
				}
				if (noise_limit_json != NULL) {
					Noise_Limit = noise_limit_json->valueint; 
					Store_Data[2] = Noise_Limit;
					Store_Save();
				}
//				if (led_json != NULL) {
//					LedMode = led_json->type;
////					led_json->type == cJSON_True ? ()
////					Led_Set(led_json->type == cJSON_True ? LED_ON1 : LED_OFF1); // 根据接收数据点亮或熄灭LED
//				}
				
				cJSON_Delete(raw_json);	
			}
			else {
				result = 1; // MQTT_UnPacketPublish失败，设置result为1
			}
			break; // 添加break
			
		case MQTT_PKT_PUBACK: // 发送Publish消息，平台回复的Ack
		
			if(MQTT_UnPacketPublishAck(cmd) == 0){
//				UsartPrintf(USART_DEBUG, "Tips: MQTT Publish Send OK\r\n");
			} else {
				result = 1; // MQTT_UnPacketPublishAck失败，设置result为1
			}		
			break;
		
		case MQTT_PKT_SUBACK: // 发送Subscribe消息的Ack
			if(MQTT_UnPacketSubscribe(cmd) == 0) {
//				UsartPrintf(USART_DEBUG, "Tips: MQTT Subscribe OK\r\n");
			} else {
//				UsartPrintf(USART_DEBUG, "Tips: MQTT Subscribe Err\r\n");
				result = 1; // MQTT_UnPacketSubscribe失败，设置result为1
			}
			break;
		
		default:
			result = 1; // 默认情况，设置result为1
			break;
	}
	
	ESP8266_Clear(); // 清空缓存
	
	if(type == MQTT_PKT_CMD || type == MQTT_PKT_PUBLISH) {
		MQTT_FreeBuffer(cmdid_topic);
		MQTT_FreeBuffer(req_payload);
	}

	return result; // 返回result
}
