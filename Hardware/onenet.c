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
*	函数名称：	OneNET_Authorization
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
//  ⚠️【已废弃】函数名称：OneNET_RegisterDevice
//
//  说明：
//    - 此函数基于 OneNET 旧版 API（/mqtt/v1/devices/reg）
//    - 存在硬编码、脆弱解析、安全风险等问题
//    - **不适用于量产或正式项目**
//    - 仅作历史参考，动态注册应通过新版 API + 安全设计实现
//    - 当前项目采用【预注册设备】方案（devid/key 硬编码于配置）
//
//  替代方案：手动在 OneNET 控制台创建设备，使用固定凭证连接
//==========================================================
#if 0  // 废弃：动态注册功能（存在安全与兼容性问题）
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
#endif

/**
 * @brief 与OneNET平台建立设备连接
 * 
 * 该函数实现设备与OneNET物联网平台的MQTT连接建立过程，
 * 包括生成鉴权Token、构建CONNECT包、发送连接请求以及处理CONNACK响应。
 * 
 * @return unsigned char 连接结果状态码
 *         - 0: 连接成功
 *         - 1: 鉴权失败
 *         - 2: MQTT包构造失败
 *         - 3: 等待响应超时
 *         - 4: 接收到非CONNACK包
 *         - 5: CONNACK数据包格式错误
 *         - 6: 协议版本不可接受
 *         - 7: 客户端标识符被拒绝
 *         - 8: 服务端不可用
 *         - 9: 用户名或密码错误
 *         - 10: 未授权连接
 *         - 11: 未知连接错误
 */
unsigned char OneNet_DevLink(void)
{
    // MQTT数据包结构体，用于存储构建的CONNECT数据包
    MQTT_PACKET_STRUCTURE mqttPacket = {NULL, 0, 0, 0};
    // 接收数据指针
    unsigned char *dataPtr;
    // 存储鉴权令牌的缓冲区
    char authorization_buf[160];
    // 返回结果，默认为鉴权失败
    unsigned char result = 1;
    // CONNACK返回码
    unsigned char connack_code = 0;

    // 记录连接开始日志
    ONENET_LOG_CONN("Starting device connection to OneNET...");

    // 步骤1: 生成鉴权Token
    // 使用OneNET的标准鉴权算法生成连接所需的认证令牌
    if (OneNET_Authorization("2018-10-31",      // API版本号
                            ONENET_PROID,       // 产品ID
                            1956499200,         // 时间戳（需根据实际情况调整）
                            ONENET_ACCESS_KEY,  // 访问密钥
                            ONENET_DEVICE_NAME, // 设备名称
                            authorization_buf,  // 输出缓冲区
                            sizeof(authorization_buf), // 缓冲区大小
                            0) != 0) {         // 其他参数
        ONENET_LOG_CONN("ERROR: Failed to generate authorization token");
        result = 1;  // 设置错误码：鉴权失败
        goto exit;   // 跳转到退出清理部分
    }

    ONENET_LOG_CONN("Authorization token generated successfully");

    // 步骤2: 构建MQTT CONNECT数据包
    // 使用产品ID、鉴权令牌、设备名等信息构建标准MQTT连接包
    if (MQTT_PacketConnect(ONENET_PROID,           // 产品ID
                          authorization_buf,      // 鉴权令牌
                          ONENET_DEVICE_NAME,     // 设备名称
                          256,                    // Keep-alive时间（秒）
                          1,                      // 清除会话标志位
                          MQTT_QOS_LEVEL0,        // QoS等级
                          NULL,                   // Will Topic（可选）
                          NULL,                   // Will Message（可选）
                          0,                      // Will消息长度
                          &mqttPacket) != 0) {    // 输出MQTT包结构
        ONENET_LOG_CONN("ERROR: Failed to construct MQTT CONNECT packet");
        result = 2;  // 设置错误码：MQTT包构造失败
        goto exit;   // 跳转到退出清理部分
    }

    ONENET_LOG_CONN("Sending MQTT CONNECT packet (%u bytes)...", mqttPacket._len);

    // 步骤3: 发送CONNECT数据包到网络
    // 通过ESP8266模块发送构建好的连接请求
    ESP8266_SendData(mqttPacket._data, mqttPacket._len);

    // 步骤4: 等待并接收CONNACK响应
    // 等待OneNET平台返回连接确认响应
    ONENET_LOG_CONN("Waiting for CONNACK response (timeout: 250ms)...");
    dataPtr = ESP8266_GetIPD(250);  // 等待250毫秒
    if (dataPtr == NULL) {
        ONENET_LOG_CONN("ERROR: Timeout waiting for CONNACK");
        result = 3;  // 设置错误码：等待响应超时
        goto exit;   // 跳转到退出清理部分
    }

    ONENET_LOG_CONN("Received response data, parsing packet...");

    // 步骤5: 验证接收到的数据包类型
    // 确认收到的是CONNACK包而不是其他类型的MQTT包
    if (MQTT_UnPacketRecv(dataPtr) != MQTT_PKT_CONNACK) {
        ONENET_LOG_CONN("ERROR: Received non-CONNACK packet");
        result = 4;  // 设置错误码：非CONNACK包
        goto exit;   // 跳转到退出清理部分
    }

    // 步骤6: 解析CONNACK包内容并处理返回码
    // 获取连接确认包中的返回码，并根据码值判断连接结果
    connack_code = MQTT_UnPacketConnectAck(dataPtr);
    ONENET_LOG_CONN("CONNACK return code: %u", connack_code);

    // 根据CONNACK返回码设置相应的错误状态
    switch (connack_code) {
        case 0:
            // 连接成功
            ONENET_LOG_CONN("Connection established successfully!");
            result = 0;
            break;
        case 1:
            // 不支持的协议版本
            ONENET_LOG_CONN("ERROR: Protocol version not accepted (code=1)");
            result = 6;
            break;
        case 2:
            // 客户端标识符被拒绝
            ONENET_LOG_CONN("ERROR: Client ID rejected (code=2)");
            result = 7;
            break;
        case 3:
            // 服务器不可用
            ONENET_LOG_CONN("ERROR: Server unavailable (code=3)");
            result = 8;
            break;
        case 4:
            // 用户名或密码错误
            ONENET_LOG_CONN("ERROR: Username or password incorrect (code=4)");
            result = 9;
            break;
        case 5:
            // 未授权连接
            ONENET_LOG_CONN("ERROR: Unauthorized connection (code=5)");
            result = 10;
            break;
        case 255:
            // CONNACK包格式错误
            ONENET_LOG_CONN("ERROR: CONNACK packet format error (invalid length or flags)");
            result = 5;
            break;
        default:
            // 未知的返回码
            ONENET_LOG_CONN("ERROR: Unknown CONNACK return code: %u", connack_code);
            result = 11;
            break;
    }

exit:
    // 释放MQTT数据包占用的内存资源
    MQTT_DeleteBuffer(&mqttPacket);
    // 返回连接结果
    return result;
}

extern u8 temp,humi;
extern u16 Dust_Limit;
extern u8 Noise_Limit;
extern u16 decibels;//当前环境中分贝大小
extern u16 PM2_5_ENV;
/**
 * @brief 生成 OneNET 平台所需的 JSON 数据包
 * @param buf       输出缓冲区
 * @param buf_size  缓冲区大小（字节）
 * @return          成功时返回写入的字符数（不含 '\0'），失败返回 -1
 *
 * @note 修改 JSON 字段时请注意：
 *       - JSON 对象中 **最后一个字段后面不能有逗号**；
 *       - 若增删字段，务必同步调整前一项末尾的逗号（`,`）；
 *       - 例如：删除最后一项时，需将其前一项末尾的逗号一并删除；
 *       - 否则将生成非法 JSON，导致 OneNET 解析失败！
 */
int OneNet_FillBuf(char *buf, size_t buf_size)
{
    if (buf == NULL || buf_size == 0) {
        return -1;
    }

    int len = snprintf(buf, buf_size,
        "{\"id\":\"123\",\"params\":{"
            "\"temp\":{\"value\":%d},"
            "\"humi\":{\"value\":%d},"
            "\"dust\":{\"value\":%d},"
            "\"noise\":{\"value\":%d},"
            "\"noise_limit\":{\"value\":%d},"
            "\"dust_limit\":{\"value\":%d},"
            "\"dust_excess\":{\"value\":%s},"      // ← 非最后一项，保留逗号
			"\"noise_excess\":{\"value\":%s}"      // ← 最后一项，**无逗号**，报警可改为事件上传而非属性
        "}}",
        temp,
        humi,
        PM2_5_ENV,
        decibels,
        Noise_Limit,
        Dust_Limit,
        (PM2_5_ENV > Dust_Limit) ? "true" : "false",
        (decibels > Noise_Limit) ? "true" : "false"
    );

    if (len < 0 || (size_t)len >= buf_size) {
        return -1;
    }

    return len;
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
    int8_t ret = -3;
    int sentBytes = 0;
    int body_len = 0;  // 注意：现在使用 int 类型（与 OneNet_FillBuf 返回值一致）

    memset(payloadBuf, 0, sizeof(payloadBuf));

    // 调用新版 OneNet_FillBuf，传入缓冲区大小
    body_len = OneNet_FillBuf(payloadBuf, sizeof(payloadBuf));

    // 检查返回值：-1 表示错误，>=0 表示成功字节数
    if (body_len <= 0) {
        ONENET_LOG_SEND("ERROR: Failed to generate payload (ret=%d)", body_len);       
        return -3;
    }

    // 可选：日志输出（注意 body_len 是 int）
    ONENET_LOG_SEND("Payload (%d bytes): %s", body_len, payloadBuf);

    // 创建 MQTT 数据包结构（仅元数据，不含 payload）
    if (MQTT_PacketSaveData(ONENET_PROID, ONENET_DEVICE_NAME, (uint16_t)body_len, NULL, &mqttPacket) != 0) {
        ONENET_LOG_SEND("ERROR: MQTT packet creation failed!");
        return -1;
    }

    // 将 payload 填入 MQTT 包
    for (int i = 0; i < body_len; i++) {  // 使用 int 索引更安全
        if (mqttPacket._len >= mqttPacket._size) {
            ONENET_LOG_SEND("ERROR: Buffer overflow during payload fill!");
            ret = -4;
            goto cleanup;
        }
        mqttPacket._data[mqttPacket._len++] = payloadBuf[i];
    }

    // 发送数据
    sentBytes = ESP8266_SendData(mqttPacket._data, mqttPacket._len);
    ONENET_LOG_SEND("Sent %d / %u bytes", sentBytes, mqttPacket._len);

    if (sentBytes != (int)mqttPacket._len) {
        ONENET_LOG_SEND("ERROR: Incomplete send!");
        ret = -2;
        goto cleanup;
    }

    ret = 0; // success

cleanup:
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


////==========================================================
// 函数名称：   OneNet_RevPro
// 功能：       处理 OneNet 平台下发的 MQTT 消息
// 入口参数：   cmd - 接收到的原始数据包（来自 ESP8266 的 +IPD 数据）
// 返回值：     0 - 成功处理；1 - 解析或处理失败
//==========================================================
u8 OneNet_RevPro(unsigned char *cmd)
{
    // 用于保存从 MQTT 包中解析出的 payload（即 JSON 内容）
    char *req_payload = NULL;
    // 用于保存从 MQTT 包中解析出的主题（topic）
    char *cmdid_topic = NULL;

    // （已注释）原本用于提取命令 ID 字符串
    // char *id_str = NULL;

    // 主题和 payload 的实际长度（单位：字节）
    unsigned short topic_len = 0;
    unsigned short req_len = 0;

    // QoS 等级（服务质量）
    unsigned char qos = 0;

    // 静态变量，用于记录最近一次的 MQTT 报文 ID（用于 PUBACK 匹配等）
    static unsigned short pkt_id = 0;

    // 当前接收到的 MQTT 报文类型（如 PUBLISH、PUBACK 等）
    unsigned char type = 0;

    // 函数最终返回结果：0 表示成功，1 表示失败
    int result = 0; // 使用 int 类型避免潜在类型问题

    // cJSON 指针：用于解析 JSON 数据
    cJSON *raw_json;        // 指向整个 JSON 根对象
    cJSON *params_json;     // 指向 "params" 子对象

    // （已注释）原用于解析 LED 控制和命令 ID
    // cJSON *led_json, *id_json;

    // 用于分别指向 JSON 中的 "dust_limit" 和 "noise_limit" 字段
    cJSON *dust_limit_json; // 解析扬尘阈值
    cJSON *noise_limit_json; // 解析噪音阈值

    // 第一步：判断接收到的是哪种 MQTT 报文类型
    type = MQTT_UnPacketRecv(cmd);

    // 根据报文类型分发处理
    switch (type)
    {
        // 情况1：收到平台下发的 PUBLISH 消息（即控制指令）
        case MQTT_PKT_PUBLISH:
        {
            // 调用 MQTT 解包函数，从原始数据中提取 topic 和 payload
            result = MQTT_UnPacketPublish(
                cmd,               // 原始数据包
                &cmdid_topic,      // 输出：主题字符串指针
                &topic_len,        // 输出：主题长度
                &req_payload,      // 输出：payload（JSON 字符串）指针
                &req_len,          // 输出：payload 长度
                &qos,              // 输出：QoS 等级
                &pkt_id            // 输出：报文 ID
            );

            // 如果解包成功（返回 0）
            if (result == 0)
            {
                // （已注释）调试打印：显示 topic 和 payload 内容
                // UsartPrintf(USART_DEBUG, "topic: %s, topic_len: %d, payload: %s, payload_len: %d\r\n",
                //             cmdid_topic, topic_len, req_payload, req_len);

                // 使用 cJSON 解析 payload 中的 JSON 字符串
                raw_json = cJSON_Parse(req_payload);
                if (raw_json == NULL)
                {
                    // JSON 格式错误，无法解析
                    // （已注释）UsartPrintf(USART_DEBUG, "Error: JSON 解析失败\r\n");
                    return 1; // 直接返回失败
                }

                // （已注释）原计划提取命令 ID（"id" 字段），但未启用
                // id_json = cJSON_GetObjectItem(raw_json, "id");
                // if (id_json != NULL && (id_json->type == cJSON_String)) {
                //     id_str = id_json->valuestring;
                // } else {
                //     result = 1; // 无效 id 字段
                // }

                // 从 JSON 根对象中获取 "params" 子对象
                params_json = cJSON_GetObjectItem(raw_json, "params");

                // 从 "params" 中分别获取 "dust_limit" 和 "noise_limit"
                dust_limit_json = cJSON_GetObjectItem(params_json, "dust_limit");
                noise_limit_json = cJSON_GetObjectItem(params_json, "noise_limit");

                // （已注释）原计划支持 LED 控制
                // led_json = cJSON_GetObjectItem(params_json, "led");

                // 如果 "dust_limit" 字段存在，则更新本地扬尘阈值
                if (dust_limit_json != NULL)
                {
                    Dust_Limit = dust_limit_json->valueint; // 读取整数值
                    Store_Data[1] = Dust_Limit;             // 存入全局存储数组
                    Store_Save();                           // 保存到非易失存储（如 EEPROM/Flash）
                }

                // 如果 "noise_limit" 字段存在，则更新本地噪音阈值
                if (noise_limit_json != NULL)
                {
                    Noise_Limit = noise_limit_json->valueint;
                    Store_Data[2] = Noise_Limit;
                    Store_Save();
                }

                // （已注释）LED 控制逻辑（未启用）
                // if (led_json != NULL) {
                //     LedMode = led_json->type;
                //     Led_Set(led_json->type == cJSON_True ? LED_ON1 : LED_OFF1);
                // }

                // 释放 cJSON 占用的内存
                cJSON_Delete(raw_json);
            }
            else
            {
                // MQTT 解包失败，标记为错误
                result = 1;
            }
            break; // 退出 switch（防止 fall-through）
        }

        // 情况2：收到平台对 PUBLISH 消息的确认（PUBACK）
        case MQTT_PKT_PUBACK:
        {
            // 尝试解析 PUBACK 报文
            if (MQTT_UnPacketPublishAck(cmd) == 0)
            {
                // （已注释）成功收到 PUBACK，可打印提示
                // UsartPrintf(USART_DEBUG, "Tips: MQTT Publish Send OK\r\n");
            }
            else
            {
                // 解析失败，标记错误
                result = 1;
            }
            break;
        }

        // 情况3：收到平台对 SUBSCRIBE 消息的确认（SUBACK）
        case MQTT_PKT_SUBACK:
        {
            if (MQTT_UnPacketSubscribe(cmd) == 0)
            {
                // （已注释）订阅成功
                // UsartPrintf(USART_DEBUG, "Tips: MQTT Subscribe OK\r\n");
            }
            else
            {
                // （已注释）订阅失败
                // UsartPrintf(USART_DEBUG, "Tips: MQTT Subscribe Err\r\n");
                result = 1;
            }
            break;
        }

        // 其他未知报文类型
        default:
            result = 1; // 标记为处理失败
            break;
    }

    // （已注释）原计划清空 ESP8266 缓冲区，但未启用
    // ESP8266_Clear();

    // 如果是 PUBLISH 或 CMD 类型的消息，需要释放 MQTT 解包时分配的内存
    // （注意：这里有个小问题：MQTT_PKT_CMD 在 switch 中并未处理，可能是历史遗留）
    if (type == MQTT_PKT_CMD || type == MQTT_PKT_PUBLISH)
    {
        MQTT_FreeBuffer(cmdid_topic);   // 释放主题字符串内存
        MQTT_FreeBuffer(req_payload);   // 释放 payload 字符串内存
    }

    // 返回处理结果：0 成功，1 失败
    return result;
}



//==========================================================
// 函数名称：   OneNet_RevPro（问题代码！）
// 功能：       处理 OneNet 平台下发的 MQTT 消息
// 入口参数：   cmd - 接收到的原始数据包
// 返回值：     0 - 成功处理；1 - 解析/处理失败
//==========================================================
//u8 OneNet_RevPro(unsigned char *cmd)
//{
//if (cmd == NULL) {
//	ONENET_LOG_PARSE("Received NULL command");
//	return 1;
//}

//char *topic = NULL;
//char *payload = NULL;
//unsigned short topic_len = 0, payload_len = 0;
//u8 type = MQTT_UnPacketRecv(cmd);

//ONENET_LOG_PARSE("MQTT packet type: %d", type);

//switch (type)
//{
//	case MQTT_PKT_PUBLISH:
//	{
//		if (MQTT_UnPacketPublish(cmd, &topic, &topic_len, &payload, &payload_len, NULL, NULL) != 0) {
//			ONENET_LOG_PARSE("Failed to unpack PUBLISH packet");
//			return 1;
//		}

//		// 打印 topic 和 payload（注意：payload 可能不含 '\0'，需小心打印）
//		if (topic && topic_len > 0) {
//			char topic_str[64] = {0};
//			memcpy(topic_str, topic, (topic_len < 63) ? topic_len : 63);
//			ONENET_LOG_PARSE("Topic: %s", topic_str);
//		}

//		if (payload && payload_len > 0) {
//			char payload_str[256] = {0};
//			memcpy(payload_str, payload, (payload_len < 255) ? payload_len : 255);
//			ONENET_LOG_PARSE("Raw Payload (%u bytes): %s", payload_len, payload_str);
//		} else {
//			ONENET_LOG_PARSE("Empty or invalid payload!");
//			MQTT_FreeBuffer(topic);
//			MQTT_FreeBuffer(payload);
//			return 1;
//		}

//		cJSON *root = cJSON_Parse(payload);
//		if (root == NULL) {
//			const char *error_ptr = cJSON_GetErrorPtr();
//			if (error_ptr) {
//				ONENET_LOG_PARSE("cJSON parse failed at: '%s'", error_ptr);
//			} else {
//				ONENET_LOG_PARSE("cJSON parse failed (unknown reason)");
//			}
//			MQTT_FreeBuffer(topic);
//			MQTT_FreeBuffer(payload);
//			return 1;
//		}

//		ONENET_LOG_PARSE("cJSON parsed successfully");

//		cJSON *params = cJSON_GetObjectItem(root, "params");
//		if (params && cJSON_IsObject(params)) {
//			ONENET_LOG_PARSE("Found 'params' object");

//			cJSON *dust_json = cJSON_GetObjectItem(params, "dust_limit");
//			cJSON *noise_json = cJSON_GetObjectItem(params, "noise_limit");

//			if (dust_json && cJSON_IsNumber(dust_json)) {
//				Dust_Limit = dust_json->valueint;
//				Store_Data[1] = Dust_Limit;
//				Store_Save();
//				ONENET_LOG_PARSE("Set dust_limit = %d", Dust_Limit);
//			} else {
//				ONENET_LOG_PARSE("dust_limit missing or not a number");
//			}

//			if (noise_json && cJSON_IsNumber(noise_json)) {
//				Noise_Limit = noise_json->valueint;
//				Store_Data[2] = Noise_Limit;
//				Store_Save();
//				ONENET_LOG_PARSE("Set noise_limit = %d", Noise_Limit);
//			} else {
//				ONENET_LOG_PARSE("noise_limit missing or not a number");
//			}
//		} else {
//			ONENET_LOG_PARSE("'params' field not found or not an object");
//		}

//		cJSON_Delete(root);
//		MQTT_FreeBuffer(topic);
//		MQTT_FreeBuffer(payload);
//		return 0;
//	}

//	case MQTT_PKT_PUBACK:
//		MQTT_UnPacketPublishAck(cmd);
//		ONENET_LOG_PARSE("Handled PUBACK");
//		return 0;

//	case MQTT_PKT_SUBACK:
//		MQTT_UnPacketSubscribe(cmd);
//		ONENET_LOG_PARSE("Handled SUBACK");
//		return 0;

//	default:
//		ONENET_LOG_PARSE("Unhandled packet type: %d", type);
//		return 1;
//}
//}

