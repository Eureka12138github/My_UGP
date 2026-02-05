/**
	************************************************************
	************************************************************
	************************************************************
	*	文件名： 	onenet_mqtt.c
	*
	*	作者： 		张继瑞、Eureka
	*
	*	日期： 		2017-05-08、2026-02-04
	*
	*	版本： 		V1.2
	*
	*	说明： 		与onenet平台的数据交互接口层
	*
	*	修改记录：	V1.0：协议封装、返回判断都在同一个文件，并且不同协议接口不同。
	*				V1.1：提供统一接口供应用层使用，根据不同协议文件来封装协议相关的内容。
	*				V1.2：重构下行消息处理逻辑，支持 OneNET 物模型属性设置（/property/set）与响应（set_reply）；
	*					  增加对 /property/post/reply 回执的解析；完善错误码体系与资源安全释放机制；
	*					  修复 MQTT PUBLISH 包在 QoS=0 时 pkt_id 传参兼容性问题；优化日志与健壮性。
	************************************************************
	************************************************************
	************************************************************
**/


#include "onenet_mqtt.h"



char devid[16];

char key[48];


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




unsigned char OneNet_DevLink(void)
{
    MQTT_PACKET_STRUCTURE mqttPacket = {NULL, 0, 0, 0};
    char authorization_buf[160];
    unsigned char result = 1;
    unsigned char connack_code = 0;
	esp8266_ipd_frame_t frame = {0};

    ONENET_LOG_CONN("Starting device connection to OneNET...");

    // 步骤1: 生成鉴权Token
    if (OneNET_Authorization("2018-10-31",
                            ONENET_PROID,
                            1956499200,
                            ONENET_ACCESS_KEY,
                            ONENET_DEVICE_NAME,
                            authorization_buf,
                            sizeof(authorization_buf),
                            0) != 0) {
        ONENET_LOG_CONN("ERROR: Failed to generate authorization token");
        result = 1;
        goto exit;
    }

    ONENET_LOG_CONN("Authorization token generated successfully");

    // 步骤2: 构建MQTT CONNECT数据包
    if (MQTT_PacketConnect(ONENET_PROID,
                          authorization_buf,
                          ONENET_DEVICE_NAME,
                          256,
                          1,
                          MQTT_QOS_LEVEL0,
                          NULL,
                          NULL,
                          0,
                          &mqttPacket) != 0) {
        ONENET_LOG_CONN("ERROR: Failed to construct MQTT CONNECT packet");
        result = 2;
        goto exit;
    }

    ONENET_LOG_CONN("Sending MQTT CONNECT packet (%u bytes)...", mqttPacket._len);

    // 步骤3: 发送CONNECT数据包
    ESP8266_SendData(mqttPacket._data, mqttPacket._len);

    // ✅ 步骤4: 等待并接收CONNACK响应（使用新接口）
    ONENET_LOG_CONN("Waiting for CONNACK response (timeout: 250ms)...");
    
    frame = ESP8266_GetIPD(250); // 阻塞等待最多250ms

    if (!frame.valid) {
        ONENET_LOG_CONN("ERROR: Timeout or invalid data waiting for CONNACK");
        result = 3;  // 超时
        goto exit;
    }


    ONENET_LOG_CONN("Received response data (%u bytes), parsing packet...", frame.len);

    // ✅ 步骤5: 验证是否为 CONNACK 包
    if (MQTT_UnPacketRecv((unsigned char*)frame.data) != MQTT_PKT_CONNACK) {
        ONENET_LOG_CONN("ERROR: Received non-CONNACK packet");
        result = 4;
        goto exit;
    }

    // ✅ 步骤6: 解析 CONNACK 返回码
    connack_code = MQTT_UnPacketConnectAck((unsigned char*)frame.data);
    ONENET_LOG_CONN("CONNACK return code: %u", connack_code);

    switch (connack_code) {
        case 0:
            ONENET_LOG_CONN("Connection established successfully!");
            result = 0;
            break;
        case 1: result = 6; ONENET_LOG_CONN("ERROR: Protocol version not accepted (code=1)"); break;
        case 2: result = 7; ONENET_LOG_CONN("ERROR: Client ID rejected (code=2)"); break;
        case 3: result = 8; ONENET_LOG_CONN("ERROR: Server unavailable (code=3)"); break;
        case 4: result = 9; ONENET_LOG_CONN("ERROR: Username or password incorrect (code=4)"); break;
        case 5: result = 10; ONENET_LOG_CONN("ERROR: Unauthorized connection (code=5)"); break;
        case 255: result = 5; ONENET_LOG_CONN("ERROR: CONNACK packet format error"); break;
        default: result = 11; ONENET_LOG_CONN("ERROR: Unknown CONNACK return code: %u", connack_code); break;
    }

exit:
    MQTT_DeleteBuffer(&mqttPacket);
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
    bool send_result = 1;
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
	if (mqttPacket._len + body_len > mqttPacket._size) {
		ONENET_LOG_SEND("ERROR: Not enough space for payload (need %d, have %u)!", 
						body_len, mqttPacket._size - mqttPacket._len);
		ret = -4;
		goto cleanup;
	}
	memcpy(&mqttPacket._data[mqttPacket._len], payloadBuf, body_len);
	mqttPacket._len += body_len;

    // 发送数据
    send_result  = ESP8266_SendData(mqttPacket._data, mqttPacket._len);
    ONENET_LOG_SEND("Already Sent %u bytes!",mqttPacket._len);

    if (send_result) {
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
    MQTT_PACKET_STRUCTURE mqtt_packet = {NULL, 0, 0, 0};
    char topic1[56], topic2[56];
    
    // 主题1：接收平台下发命令
    snprintf(topic1, sizeof(topic1), "$sys/%s/%s/thing/property/set", ONENET_PROID, ONENET_DEVICE_NAME);
    // 主题2：接收平台对上报数据的回执
    snprintf(topic2, sizeof(topic2), "$sys/%s/%s/thing/property/post/reply", ONENET_PROID, ONENET_DEVICE_NAME);
    
    const char *topics[] = {topic1, topic2};
    
    if (MQTT_PacketSubscribe(MQTT_SUBSCRIBE_ID, MQTT_QOS_LEVEL1, topics, 2, &mqtt_packet) == 0)
    {
        ESP8266_SendData(mqtt_packet._data, mqtt_packet._len);
        MQTT_DeleteBuffer(&mqtt_packet);
    }
}


/**
 * @brief OneNET平台下行消息处理函数
 * 
 * 该函数负责解析和处理来自OneNET平台的MQTT下行消息，包括：
 * 1. 属性设置指令 (/thing/property/set)
 * 2. 属性上报响应 (/thing/property/post/reply)
 * 3. 其他MQTT控制包 (PUBACK, SUBACK等)
 * 
 * @param[in] cmd 指向MQTT接收数据包的指针
 * @return u8 返回处理结果状态码
 *         - ONENET_OK: 处理成功
 *         - ONENET_PARSE_ERR: 解析错误
 *         - ONENET_SET_HANDLED: 属性设置指令已处理
 *         - ONENET_POST_SUCCESS: 属性上报成功
 *         - ONENET_POST_FAILED: 属性上报失败
 * 
 * @note 该函数采用goto语句统一资源清理，确保内存安全释放
 * @note 支持QoS 0的快速响应机制
 */
u8 OneNet_RevPro(unsigned char *cmd)
{
    // ==================== 变量声明 ====================
    char *req_payload = NULL;           ///< MQTT消息载荷数据
    char *cmdid_topic = NULL;           ///< MQTT主题字符串
    unsigned short topic_len = 0;       ///< 主题长度
    unsigned short req_len = 0;         ///< 请求数据长度
    unsigned char qos = 0;              ///< QoS服务质量等级
    static unsigned short pkt_id = 0;   ///< 包标识符(静态变量)
    unsigned char type = 0;             ///< MQTT包类型
    u8 result = ONENET_OK;              ///< 函数返回结果，默认成功

    // JSON对象指针声明
    cJSON *raw_json = NULL;             ///< 原始JSON数据
    cJSON *params_json = NULL;          ///< 参数JSON对象
    cJSON *dust_limit_json = NULL;      ///< 粉尘限制值JSON对象
    cJSON *noise_limit_json = NULL;     ///< 噪声限制值JSON对象
    cJSON *id_json = NULL;              ///< 请求ID JSON对象
    cJSON *msg_json = NULL;             ///< 消息内容JSON对象
    cJSON *code_json = NULL;            ///< 状态码JSON对象

    // ==================== 包类型识别 ====================
    type = MQTT_UnPacketRecv(cmd);      // 解析MQTT包类型

    // ==================== 消息分发处理 ====================
    switch (type)
    {
        // -------------------- PUBLISH消息处理 --------------------
        case MQTT_PKT_PUBLISH:
        {
            // 解析PUBLISH包基本信息
            int unpack_result = MQTT_UnPacketPublish(
                cmd,
                &cmdid_topic,
                &topic_len,
                &req_payload,
                &req_len,
                &qos,
                &pkt_id
            );

            // 包解析失败处理
            if (unpack_result != 0)
            {
                result = ONENET_PARSE_ERR;
                goto cleanup;
            }

            // 解析JSON载荷
            raw_json = cJSON_Parse(req_payload);
            if (raw_json == NULL)
            {
                result = ONENET_PARSE_ERR;
                goto cleanup;
            }

            // ==================== 主题路由处理 ====================
            // 预定义主题后缀常量
            const char *set_suffix = "/thing/property/set";           // 属性设置
            const char *post_suffix = "/thing/property/post/reply";   // 属性上报响应
            int topic_len_int = (int)topic_len;
            int set_len = strlen(set_suffix);
            int post_len = strlen(post_suffix);

            // -------------------- 属性设置指令处理 --------------------
            if (topic_len_int >= set_len &&
                strcmp(cmdid_topic + topic_len_int - set_len, set_suffix) == 0)
            {
                // 解析参数对象
                params_json = cJSON_GetObjectItem(raw_json, "params");
                if (params_json == NULL || !cJSON_IsObject(params_json))
                {
                    ONENET_LOG_PARSE("⚠️ Invalid or missing 'params' in set command");
                    result = ONENET_PARSE_ERR;
                    goto cleanup;
                }

                // 提取并更新粉尘限制值
                dust_limit_json = cJSON_GetObjectItem(params_json, "dust_limit");
                if (dust_limit_json && cJSON_IsNumber(dust_limit_json))
                {
                    Dust_Limit = (int)dust_limit_json->valuedouble;
                    Store_Data[DUST_LIMIT_STORE_IDX] = Dust_Limit;
                    Store_Save();  // 保存到存储
                }

                // 提取并更新噪声限制值
                noise_limit_json = cJSON_GetObjectItem(params_json, "noise_limit");
                if (noise_limit_json && cJSON_IsNumber(noise_limit_json))
                {
                    Noise_Limit = (int)noise_limit_json->valuedouble;
                    Store_Data[NOISE_LIMIT_STORE_IDX] = Noise_Limit;
                    Store_Save();  // 保存到存储
                }

                // 构造并发送属性设置响应
                char reply_topic[64];      // 响应主题缓冲区
                char reply_payload[128];   // 响应载荷缓冲区
                MQTT_PACKET_STRUCTURE reply_packet = {NULL, 0, 0, 0};  // 响应包结构

                // 构造响应主题
                snprintf(reply_topic, sizeof(reply_topic),
                         "$sys/%s/%s/thing/property/set_reply",
                         ONENET_PROID, ONENET_DEVICE_NAME);

                // 获取请求ID，若不存在则使用默认值
                id_json = cJSON_GetObjectItem(raw_json, "id");
                const char *req_id = (id_json && cJSON_IsString(id_json)) ? 
                                   id_json->valuestring : "123";

                // 构造响应JSON
                snprintf(reply_payload, sizeof(reply_payload),
                         "{\"id\":\"%s\",\"code\":200,\"msg\":\"success\"}",
                         req_id);

                ONENET_LOG_PARSE("Sending set_reply: %s", reply_payload);

                // 发送MQTT响应包(QoS 0 - 快速响应)
                if (MQTT_PacketPublish(0,                    // Packet ID (QoS 0无需)
                                       reply_topic,          // 响应主题
                                       reply_payload,        // 响应载荷
                                       strlen(reply_payload), // 载荷长度
                                       MQTT_QOS_LEVEL0,      // QoS 0 - 最多一次
                                       0,                    // retain标志
                                       1,                    // own标志
                                       &reply_packet) == 0)  // 包结构指针
                {
                    ESP8266_SendData(reply_packet._data, reply_packet._len);
                    MQTT_DeleteBuffer(&reply_packet);  // 释放包内存
                }
                else
                {
                    ONENET_LOG_PARSE("ERROR: Failed to create set_reply packet!");
                }

                result = ONENET_SET_HANDLED;  // 设置处理完成状态
            }
            
            // -------------------- 属性上报响应处理 --------------------
            else if (topic_len_int >= post_len &&
                     strcmp(cmdid_topic + topic_len_int - post_len, post_suffix) == 0)
            {
                // 解析响应消息和状态码
                msg_json = cJSON_GetObjectItem(raw_json, "msg");
                code_json = cJSON_GetObjectItem(raw_json, "code");

                // 验证响应格式有效性
                if (msg_json && code_json && 
                    cJSON_IsString(msg_json) && cJSON_IsNumber(code_json))
                {
                    // 判断上报是否成功
                    if (strcmp(msg_json->valuestring, "success") == 0 && 
                        code_json->valueint == 200)
                    {
                        ONENET_LOG_PARSE("[OK] Property post success");
                        result = ONENET_POST_SUCCESS;
                    }
                    else
                    {
                        ONENET_LOG_PARSE("[ERR] Property post failed! code=%d, msg=%s",
                                         code_json->valueint, msg_json->valuestring);
                        result = ONENET_POST_FAILED;
                    }
                }
                else
                {
                    ONENET_LOG_PARSE("[WARN] Cannot parse post/reply response");
                    result = ONENET_PARSE_ERR;
                }
            }
            
            // -------------------- 其他主题处理 --------------------
            else
            {
                // 无特殊处理需求，视为协议处理成功
                result = ONENET_OK;
            }
            break;
        }

        // -------------------- PUBACK确认包处理 --------------------
        case MQTT_PKT_PUBACK:
        {
            if (MQTT_UnPacketPublishAck(cmd) == 0)
            {
                result = ONENET_OK;
            }
            else
            {
                result = ONENET_PARSE_ERR;
            }
            break;
        }

        // -------------------- SUBACK订阅确认处理 --------------------
        case MQTT_PKT_SUBACK:
        {
            if (MQTT_UnPacketSubscribe(cmd) == 0)
            {
                result = ONENET_OK;
            }
            else
            {
                result = ONENET_PARSE_ERR;
            }
            break;
        }

        // -------------------- 未知包类型处理 --------------------
        default:
            result = ONENET_PARSE_ERR;
            break;
    }

    // ==================== 统一资源清理 ====================
cleanup:
    // 仅在处理PUBLISH消息时需要清理资源
    if (type == MQTT_PKT_PUBLISH)
    {
        if (raw_json) {
            cJSON_Delete(raw_json);        // 释放JSON对象
        }
        if (cmdid_topic) {
            MQTT_FreeBuffer(cmdid_topic);  // 释放主题内存
        }
        if (req_payload) {
            MQTT_FreeBuffer(req_payload);  // 释放载荷内存
        }
    }

    return result;  // 返回处理结果
}


