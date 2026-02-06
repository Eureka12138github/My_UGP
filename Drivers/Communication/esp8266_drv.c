/**
 ************************************************************
 * 文件名：    esp8266_drv.c
 * 作者：      Eureka
 * 日期：      2026-01-29
 * 说明：      ESP8266 驱动（基于环形缓冲区串口，无全局接收缓存）
 ************************************************************
 */
 
#include "esp8266_drv.h"

/**
 ******************************************************************************
 * @brief  初始化 ESP8266 模块所需的底层硬件资源
 *
 * @details 根据预处理器宏定义，自动配置以下可选硬件：
 *          - 串口（USART1 / USART2 / USART3）用于 AT 指令通信
 *          - 复位引脚（RST）用于硬件复位控制（若定义）
 *
 * @note   - 串口波特率由 BSP_ESP8266_BAUDRATE 宏指定（通常为 115200 或 9600）
 *         - 若未定义任何 USART 宏，则仅初始化复位引脚（若存在）
 *         - 若未定义 BSP_ESP8266_RST_PIN，则跳过 GPIO 复位引脚初始化
 *
 * @warning 此函数仅完成**硬件外设初始化**，不包含 ESP8266 的软件协议握手（如 AT 测试）。
 *          软件初始化需调用 ESP8266_Init() 等高层函数。
 *
 * @pre    用户必须在工程中正确定义以下宏之一（或多个）以启用对应功能：
 *         - ESP8266_USE_USART1 / _USART2 / _USART3：选择通信串口
 *         - BSP_ESP8266_RST_PIN 与 BSP_ESP8266_RST_PORT：指定复位引脚
 *         - BSP_ESP8266_GPIO_CLK：复位引脚所在 GPIO 时钟（如 RCC_APB2Periph_GPIOA）
 *         - BSP_ESP8266_BAUDRATE：通信波特率（如 115200U）
 *
 * @retval None
 ******************************************************************************
 */
void ESP8266_HardwareInit(void)
{
#ifdef ESP8266_USE_USART1
    Usart1_Init(BSP_ESP8266_BAUDRATE);
#elif defined(ESP8266_USE_USART2)
    Usart2_Init(BSP_ESP8266_BAUDRATE);
#elif defined(ESP8266_USE_USART3)
    Usart3_Init(BSP_ESP8266_BAUDRATE);
#endif

#ifdef BSP_ESP8266_RST_PIN
    RCC_APB2PeriphClockCmd(BSP_ESP8266_GPIO_CLK, ENABLE);

    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Pin = BSP_ESP8266_RST_PIN;
    GPIO_Init(BSP_ESP8266_RST_PORT, &GPIO_InitStructure);

    GPIO_SetBits(BSP_ESP8266_RST_PORT, BSP_ESP8266_RST_PIN); // 拉高使能
#endif
}

/**
 ******************************************************************************
 * @brief  向 ESP8266 发送 AT 指令并等待指定响应字符串
 *
 * @param  cmd: 要发送的 AT 指令（必须以 "\r\n" 结尾，例如 "AT\r\n"）
 * @param  res: 期望在响应中出现的关键子串（如 "OK", "ERROR", "WIFI GOT IP"）
 *
 * @retval 0   表示成功（在 2 秒超时前收到包含 `res` 的响应）
 * @retval 1   表示失败（原因可能包括：超时、接收缓冲区未初始化、未匹配到响应）
 *
 * @note   - 函数内部会自动清空接收环形缓冲区，确保仅处理本次命令的新响应
 *         - 超时时间固定为 2000ms（2 秒），适用于绝大多数 ESP8266 AT 指令
 *         - 每 10ms 检查一次接收数据
 *         - 响应匹配使用 `strstr()` 进行子串搜索，不要求整行完全匹配
 *         - 返回值遵循嵌入式驱动惯例：0 = 成功，非 0 = 失败
 *
 * @warning 此函数为**阻塞式调用**，在成功或超时前不会返回。
 *          ❌ 严禁在高优先级中断服务程序（ISR）中调用！
 *
 * @pre    ⚠️【必须前提】以下条件必须满足，否则行为未定义：
 *         1. 已调用 `Delay_Init()` —— 确保 SysTick 正常工作（否则会死循环！）
 *         2. 已调用 `ESP8266_HardwareInit()` —— 确保通信串口已初始化
 *         3. `BSP_ESP8266_USARTx` 宏已在 `bsp_config.h` 中正确定义
 *
 * @debug  🔧【调试支持说明】
 *         本函数支持两级调试日志，通过条件编译控制，默认全部关闭：
 *
 *         ▸ 启用命令日志（查看发送内容）：
 *             在 `debug_config.h` 中取消注释：
 *                 #define ESP8266_DEBUG_CMD
 *
 *         ▸ 启用失败日志（查看超时/错误详情）：
 *             在 `debug_config.h` 中取消注释：
 *                 #define ESP8266_DEBUG_FAIL
 *
 *         ✅ 推荐开发阶段同时开启 `ESP8266_DEBUG_CMD` 和 `ESP8266_DEBUG_FAIL`，
 *            可清晰看到“发了什么”、“收到了什么”、“为何失败”。
 *
 *         ⚠️ 发布固件前务必注释掉所有 `ESP8266_DEBUG_*` 宏，
 *            以消除日志开销、节省 Flash 并防止信息泄露。
 *
 *         📌 日志输出端口由 `USART_DEBUG` 宏指定，
 *            请确保该串口已在 `main()` 中完成初始化。
 ******************************************************************************
 */
_Bool ESP8266_SendCmd(const char *cmd, const char *res)
{
    cbuf_handle_t rx_cbuf = BSP_USARTX_GetRxCbuf(BSP_ESP8266_USARTx);
    if (!rx_cbuf) {
        ESP8266_LOG_FAIL("RX buffer not initialized!");
        return 1;
    }

    circular_buf_reset(rx_cbuf);

    /* 记录发送的指令与期望响应（仅当 ESP8266_DEBUG_CMD 开启时生效） */
    ESP8266_LOG_CMD("Sending: %s", cmd);
    ESP8266_LOG_CMD("Expecting: '%s'", res ? res : "(null)");

    Serial_SendString(BSP_ESP8266_USARTx, cmd);

    uint32_t start = SysTick_Get();
    while ((SysTick_Get() - start) < 2000) {
        size_t avail = circular_buf_size(rx_cbuf);
        if (avail > 0) {
            uint8_t temp[128];
            size_t len = (avail > sizeof(temp) - 1) ? sizeof(temp) - 1 : avail;
            circular_buf_peek(rx_cbuf, temp, len);
            temp[len] = '\0';

            if (strstr((char*)temp, res) != NULL) {
                ESP8266_LOG_CMD("Matched: '%s'", res);
                return 0; // 成功
            }
        }
        Delay_ms(10);
    }

    /* 超时：打印实际接收到的内容（极有助于排错） */
    size_t final_avail = circular_buf_size(rx_cbuf);
    if (final_avail > 0) {
        uint8_t final_buf[128];
        size_t n = (final_avail > sizeof(final_buf) - 1) ? sizeof(final_buf) - 1 : final_avail;
        circular_buf_peek(rx_cbuf, final_buf, n);
        final_buf[n] = '\0';
        ESP8266_LOG_FAIL("Timeout! Got: [%s]", (char*)final_buf);
    } else {
        ESP8266_LOG_FAIL("Timeout! No response.");
    }

    return 1;
}

/**
 * @brief 从 ESP8266 获取 SNTP 时间（仿旧版逻辑，适配环形缓冲区）
 *
 * 发送 "AT+CIPSNTPTIME?"，等待响应。
 * 若响应中 **不包含 "1970"** 且 **包含 "+CIPSNTPTIME:"**，则解析时间。
 * 此逻辑与旧版 ESP8266_SNTP_Time 一致。
 *
 * @param[out] TimeStructure RTC 结构体指针
 * @param[in]  timeout_ms    总超时时间（毫秒）
 * @retval 0 成功
 * @retval 1 超时或失败
 */
u8 ESP8266_GetSNTPTime(MYRTC* TimeStructure, uint16_t timeout_ms)
{
    if (!TimeStructure) {
        ESP8266_LOG_FAIL("TimeStructure is NULL");
        return 1;
    }

    static const char* wday_map[] = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
    static const char* month_map[] = {"Jan","Feb","Mar","Apr","May","Jun",
                                      "Jul","Aug","Sep","Oct","Nov","Dec"};

    cbuf_handle_t rx_cbuf = BSP_USARTX_GetRxCbuf(BSP_ESP8266_USARTx);
    if (!rx_cbuf) {
        ESP8266_LOG_FAIL("RX buffer not initialized!");
        return 1;
    }

    // 清空旧数据
    circular_buf_reset(rx_cbuf);
    Serial_SendString(BSP_ESP8266_USARTx, "AT+CIPSNTPTIME?\r\n");

    uint32_t start = SysTick_Get();
    char line[128] = {0};
    size_t line_pos = 0;

    while ((SysTick_Get() - start) < timeout_ms) {
        uint8_t ch;
        // 尝试从环形缓冲区取一个字节
        if (circular_buf_get(rx_cbuf, &ch) == 0) {
            // 成功取到一个字节
            if (ch == '\n' || ch == '\r') {
                // 遇到行结束符，处理当前行
                if (line_pos > 0) {
                    line[line_pos] = '\0';

                    ESP8266_LOG_IPD("Line: %s", line);

                    // 检查是否是有效的 +CIPSNTPTIME 响应且年份 > 1970
                    if (strstr(line, "+CIPSNTPTIME:") != NULL) {
                        char timeStr[64] = {0};
                        uint16_t year = 0;
                        uint8_t day = 0, hour = 0, minute = 0, second = 0;

                        int parsed = sscanf(line,
                            "+CIPSNTPTIME:%3s %3s %hhu %hhu:%hhu:%hhu %hu",
                            timeStr, timeStr + 4,
                            &day, &hour, &minute, &second, &year);

                        if (parsed >= 5 && year > 1970) {
                            TimeStructure->Year = year;
                            TimeStructure->Day = day;
                            TimeStructure->Hour = hour;
                            TimeStructure->Minute = minute;
                            TimeStructure->Second = second;

                            // 解析星期
                            TimeStructure->wday = 0;
                            for (int i = 0; i < 7; i++) {
                                if (strncmp(timeStr, wday_map[i], 3) == 0) {
                                    TimeStructure->wday = i;
                                    break;
                                }
                            }

                            // 解析月份
                            TimeStructure->Month = 1;
                            for (int i = 0; i < 12; i++) {
                                if (strncmp(timeStr + 4, month_map[i], 3) == 0) {
                                    TimeStructure->Month = i + 1;
                                    break;
                                }
                            }

                            return 0; // 成功
                        }
                        // 如果是 1970，继续等待下一行
                    }
                    // 忽略其他行（如 "OK", 回显等）
                }
                line_pos = 0; // 重置行缓冲区
            } else {
                // 普通字符，存入行缓冲区
                if (line_pos < sizeof(line) - 1) {
                    line[line_pos++] = (char)ch;
                }
                // 如果行太长，自动截断（安全）
            }
        } else {
            // 缓冲区为空，稍等
            Delay_ms(5);
        }
    }

    ESP8266_LOG_FAIL("Get SNTP time timeout");
    return 1;
}


/**
 * @brief 更新 ESP8266 初始化状态到 OLED 指定区域
 *
 * 清除固定区域 (66,32) 宽 50 高 16 像素，并显示状态字符串。
 * 用于在无串口调试时提供用户反馈。
 *
 * @param[in] msg 要显示的状态信息（如 "AT ERR", "WIFI OK"）
 */
 static void ESP8266_SetStatus(const char* msg) {
    OLED_ClearArea(66, 32, 50, 16);
    OLED_ShowString(66, 32, msg, OLED_7X12_HALF);
}

/**
 ******************************************************************************
 * @brief  初始化 ESP8266 模块（AT 指令握手 + 基础配置 + SNTP 时间同步）
 *
 * @details 执行以下初始化步骤：
 *          1. 发送 "AT" 测试通信是否正常
 *          2. 设置工作模式为 STA（AT+CWMODE=1）
 *          3. 启用 DHCP 自动获取 IP（AT+CWDHCP=1,1）
 *          4. 连接预设的 WiFi 网络（通过 WIFI_CONNECT_CMD 宏定义）
 *          5. 配置 SNTP 服务器（需发送两次以激活）
 *          6. 获取网络时间并填充全局 Time 结构体
 *
 * @retval 0: 成功完成全部初始化
 * @retval 1: AT 指令无响应（通信失败）
 * @retval 2: STA 模式设置失败
 * @retval 3: DHCP 启用失败
 * @retval 4: WiFi 连接失败（未收到 "GOT IP"）
 * @retval 5: SNTP 配置失败（第二次命令未返回 OK）
 * @retval 6: SNTP 时间获取失败（多次重试后仍返回 1970 或超时）
 *
 * @note   - 本函数依赖以下前提已满足：
 *           • 已调用 Delay_Init()（SysTick 正常工作）
 *           • 已调用 ESP8266_HardwareInit()（串口与复位引脚已初始化）
 *           • WIFI_CONNECT_CMD 和 ESP8266_SNTP_CONFIG 宏已在头文件中正确定义
 *           • 全局变量 `MYRTC Time` 可被写入（用于存储获取的时间）
 *
 * @warning 调试日志通过条件编译控制，默认关闭。若需查看初始化流程，请按以下步骤操作：
 *          1. 打开头文件 `debug_config.h`
 *          2. 取消注释以下宏定义：
 *             - `#define ESP8266_DEBUG_INIT`  → 显示初始化阶段日志（推荐开启）
 *          3. 确保 `USART_DEBUG` 已被修改为调试串口用于输出
 *          4. OLED 状态提示始终生效，用于无串口场景的用户反馈
 *
 *          ⚠️ 注意：调试日志仅用于开发阶段！发布固件前请务必关闭所有 ESP8266_DEBUG_* 宏，
 *                   以避免占用 Flash 空间、泄露信息或影响性能。
 ******************************************************************************
 */
u8 ESP8266_Init(void)
{
    const u8 maxRetries = 5;
    u8 retryCount;

    ESP8266_LOG_INIT("=== ESP8266 初始化开始 ===");

    /* 清除 OLED 状态区（准备显示新状态） */
    ESP8266_SetStatus(""); // 清空状态显示

    /* 1. 测试 AT */
    ESP8266_LOG_INIT("1. 测试 AT 指令...");
    for (retryCount = 0; retryCount < maxRetries; retryCount++) {
        if (ESP8266_SendCmd("AT\r\n", "OK") == 0) {
            ESP8266_LOG_INIT("✅ AT 指令响应正常");
            break;
        }
        Delay_ms(500);
    }
    if (retryCount >= maxRetries) {
        ESP8266_LOG_INIT("❌ AT 指令无响应，初始化失败！");
        ESP8266_SetStatus("AT ERR");
        return 1;
    }

    /* 2. 设置 STA 模式 */
    ESP8266_LOG_INIT("2. 设置 STA 模式 (AT+CWMODE=1)...");
    for (retryCount = 0; retryCount < maxRetries; retryCount++) {
        if (ESP8266_SendCmd("AT+CWMODE=1\r\n", "OK") == 0) {
            ESP8266_LOG_INIT("✅ STA 模式设置成功");
            break;
        }
        Delay_ms(500);
    }
    if (retryCount >= maxRetries) {
        ESP8266_LOG_INIT("❌ 设置 STA 模式失败！");
        ESP8266_SetStatus("STA ERR");
        return 2;
    }

    /* 3. 启用 DHCP */
    ESP8266_LOG_INIT("3. 启用 DHCP (AT+CWDHCP=1,1)...");
    for (retryCount = 0; retryCount < maxRetries; retryCount++) {
        if (ESP8266_SendCmd("AT+CWDHCP=1,1\r\n", "OK") == 0) {
            ESP8266_LOG_INIT("✅ DHCP 已启用");
            break;
        }
        Delay_ms(500);
    }
    if (retryCount >= maxRetries) {
        ESP8266_LOG_INIT("❌ 启用 DHCP 失败！");
        ESP8266_SetStatus("DHCP ERR");
        return 3;
    }

    /* 4. 连接 WiFi */
    ESP8266_LOG_INIT("4. 连接 WiFi (使用预设 SSID/PWD)...");
    for (retryCount = 0; retryCount < maxRetries; retryCount++) {
        if (ESP8266_SendCmd(WIFI_CONNECT_CMD, "GOT IP") == 0) {
            ESP8266_LOG_INIT("✅ WiFi 连接成功，已获取 IP");
            break;
        }
        Delay_ms(500);
    }
    if (retryCount >= maxRetries) {
        ESP8266_LOG_INIT("❌ WiFi 连接失败，请检查 SSID/密码或信号！");
        ESP8266_SetStatus("WIFI ERR");
        return 4;
    }

    /* 5. 配置 SNTP（必须发送两次以确保激活） */
    ESP8266_LOG_INIT("🔧 配置 SNTP 服务器...");

    // 第一次：设置配置（部分固件不返回 OK，可容忍）
    if (ESP8266_SendCmd(ESP8266_SNTP_CONFIG, "OK") != 0) {
        ESP8266_LOG_INIT("⚠️ 第一次 SNTP 配置未收到 OK（可能正常）");
    }
    Delay_ms(100); // 防指令粘连

    // 第二次：激活同步（必须成功）
    if (ESP8266_SendCmd(ESP8266_SNTP_CONFIG, "OK") != 0) {
        ESP8266_LOG_INIT("❌ 第二次 SNTP 配置失败");
        ESP8266_SetStatus("SNTP ERR");
        return 5;
    }
    ESP8266_LOG_INIT("✅ SNTP 已激活");

    /* 6. 获取网络时间（带重试） */
    ESP8266_LOG_INIT("⏳ 尝试获取网络时间...");
    u8 time_retry;
    for (time_retry = 0; time_retry < maxRetries; time_retry++) {
        if (ESP8266_GetSNTPTime(&Time, 5000) == 0) {
            ESP8266_LOG_INIT("✅ 时间获取成功");
            break;
        }
        Delay_ms(500);
    }
    if (time_retry >= maxRetries) {
        ESP8266_LOG_INIT("❌ 时间获取失败");
        ESP8266_SetStatus("TIME ERR");
        return 6;
    }

    ESP8266_LOG_INIT("=== ESP8266 初始化完成！===");
    return 0;
}


/**
 * @brief 向 ESP8266 提交待发送的数据（非阻塞式提交）
 *
 * 本函数用于在已建立 TCP/UDP 连接后，向 ESP8266 模块提交应用层数据。
 * 它执行以下操作：
 *   1. 发送 AT+CIPSEND=<len> 指令
 *   2. 等待 ESP8266 返回 ">" 提示符（表示准备接收数据）
 *   3. 通过串口直接发送用户数据载荷
 *
 * @param[in] data 指向待发送数据的缓冲区（非空）
 * @param[in] len  数据长度（字节），必须 > 0 且 ≤ 2048（ESP8266 单次限制）
 *
 * @retval 0 成功提交数据到 ESP8266（已进入发送队列）
 * @retval 1 失败（原因：AT+CIPSEND 超时、未收到 ">" 提示符等）
 *
 * @note
 *   - **本函数不等待 "SEND OK" 或网络 ACK 响应**。  
 *     原因：ESP8266 的发送确认（如 "SEND OK"）和平台业务响应（如 OneNET 的 4 字节 ACK）  
 *     均以**异步主动上报**，（URC）形式返回（例如 +IPD 或纯数据帧），  
 *     应由主循环持续调用 `ESP8266_GetIPD()` 统一捕获和处理。
 *   - 此设计实现**发送与接收解耦**，避免因网络延迟阻塞主业务逻辑。
 *   - 成功返回仅表示数据已成功提交至 ESP8266 内部缓冲区，**不代表对端已接收**。
 *
 * @warning
 *   - 调用前必须确保 ESP8266 已成功连接目标服务器（如通过 AT+CIPSTART）
 *   - 若 ESP8266 缓冲区满或链路异常，后续可能通过 URC 上报错误（如 "SEND FAIL"），
 *     但本函数无法感知，需依赖上层超时或状态机检测。
 */
bool ESP8266_SendData(const unsigned char *data, unsigned short len)
{
    char cmdBuf[32];
    snprintf(cmdBuf, sizeof(cmdBuf), "AT+CIPSEND=%d\r\n", len);
    
    // 发送 AT+CIPSEND 命令，等待 ">" 提示符
    if (ESP8266_SendCmd(cmdBuf, ">")) {
        return 1;
    }
	
    Serial_SendArray(BSP_ESP8266_USARTx, data, len);
    return 0; // 成功提交数据。但响应不在此判断！
}


/**
 * @brief 根据数据内容识别 ESP8266 +IPD 接收帧的业务类型
 *
 * 本函数用于在成功解析出完整 +IPD 数据载荷后，根据其内容特征判断其语义类型，
 * 以便上层应用进行差异化处理（如忽略 ACK、解析 OneNET 指令等）。
 *
 * @param[in] data 指向已接收数据载荷的指针（非空）
 * @param[in] len  数据长度（字节），应 > 0
 * @return         识别出的数据类型，可能为：
 *                 - ESP8266_IPD_TYPE_ACK: 平台返回的 4 字节确认帧 (0x40 0x02 0x00 0x0A)
 *                 - ESP8266_IPD_TYPE_ONENET_CMD: OneNET 下发的 JSON 控制指令
 *                 - ESP8266_IPD_TYPE_CUSTOM: 其他有效业务数据
 *                 - ESP8266_IPD_TYPE_UNKNOWN: 空数据或无法识别内容
 *
 * @note 该函数不修改输入数据，仅做只读分析。
 * @warning 不对 data 指针做 NULL 检查，调用者需确保有效性。
 */
static esp8266_ipd_type_t classify_ipd_data(const uint8_t *data, uint16_t len)
{
    // 1. 判断是否为已知 ACK 帧：固定 4 字节 {0x40, 0x02, 0x00, 0x0A}
    if (len == 4U &&
        data[0] == 0x40U &&
        data[1] == 0x02U &&
        data[2] == 0x00U &&
        data[3] == 0x0AU) {
        return ESP8266_IPD_TYPE_ACK;
    }

    // 2. 判断是否为 OneNET 控制指令：长度合理且包含 JSON 起始符 '{'
    if ((len >= 10U) && (len <= 255U)) {
        if (memchr(data, '{', len) != NULL) {
            // 可扩展：未来可增加关键字匹配（如 "params", "$sys"）提升准确性
            return ESP8266_IPD_TYPE_ONENET_CMD;
        }
    }

    // 3. 其他非空数据视为自定义业务帧；空数据视为未知
    return (len > 0U) ? ESP8266_IPD_TYPE_CUSTOM : ESP8266_IPD_TYPE_UNKNOWN;
}


/**
 * @brief 解析 ESP8266 的 +IPD 数据帧（支持超时控制）
 *
 * 该函数通过状态机从 USART 接收环形缓冲区中识别并提取完整的 +IPD 数据载荷。
 * 支持单连接与多连接模式（自动跳过 conn_id），并根据内容分类为 ACK、OneNET 指令等类型。
 *
 * @param[in] timeout_ms 超时时间（毫秒）：
 *                       - = 0：非阻塞模式，若无完整帧则立即返回
 *                       - > 0：阻塞等待，最长等待 timeout_ms 毫秒
 *
 * @return esp8266_ipd_frame_t 解析结果结构体，包含：
 *         - .data:  指向内部静态缓冲区的数据指针（**仅在下次调用前有效**）
 *         - .len:   接收到的数据长度（字节）
 *         - .type:  数据类型（ UNKNOWN / ACK / ONENET_CMD / CUSTOM ）
 *         - .valid: 是否成功解析出有效帧（true 表示有效）
 *
 * @note
 *   - 内部使用静态缓冲区（大小由 IPD_BUFFER_SIZE 定义，默认 ≥256 字节）
 *   - 返回的 .data 指针**不可跨调用持有**，必须在本次返回后立即处理或复制
 *   - 函数具有状态记忆能力，需在主循环中持续调用直至返回 valid=true 或超时
 *   - 遇到超时、无效输入或解析错误时，会自动重置状态机，保证下次调用干净启动
 *   - 自动忽略 ESP8266 主动上报的异步消息（如 +CME ERROR、+CWJAP_CUR 等 +C... 帧）
 *
 * @par 调试说明
 *   定义 ESP8266_DEBUG_IPD 可开启详细日志。典型有效输出示例（ACK 帧）：
 *   @code
 *   [ESP8266][IPD] Detected '+'
 *   [ESP8266][IPD] Start parsing length, first digit: 4
 *   [ESP8266][IPD] Parsed length: 4
 *   [ESP8266][IPD] Received 4 bytes:
 *   40 02 00 0A 
 *   [ESP8266][IPD] Classified as: ACK
 *   @endcode
 *   注意：中间状态（如 GOT_I/GOT_P）仅在出错时打印日志，成功路径保持静默。
 *
 * @warning 本函数非线程安全，多任务环境下需加互斥锁保护
 */
esp8266_ipd_frame_t ESP8266_GetIPD(uint16_t timeout_ms)
{
    /* ------------------------ 状态机持久化变量 ------------------------ */
    static enum {
        STATE_IDLE,
        STATE_GOT_PLUS,
        STATE_GOT_I,
        STATE_GOT_P,
        STATE_GOT_D,
        STATE_AFTER_D,
        STATE_PARSING_LEN,
        STATE_RECEIVING_DATA
    } state = STATE_IDLE;

	
    static uint16_t expected_len = 0U;
    static uint16_t received_len = 0U;
	static uint8_t data_buf[IPD_BUFFER_SIZE];
    static char     len_str[6] = {0};
    static uint8_t  len_idx = 0U;

    /* ------------------------ 初始化 ------------------------ */
    cbuf_handle_t rx_cbuf = BSP_USARTX_GetRxCbuf(BSP_ESP8266_USARTx);
    if (!rx_cbuf) {
        ESP8266_LOG_IPD("RX buffer not ready");
        return (esp8266_ipd_frame_t){ .valid = false };
    }

    const uint32_t start_tick = SysTick_Get();

    /* ------------------------ 主循环 ------------------------ */
    while (1) {
        // 超时检测
        if ((timeout_ms > 0U) && ((SysTick_Get() - start_tick) >= timeout_ms)) {
            if (state != STATE_IDLE) {
                ESP8266_LOG_IPD("Timeout in state %d, resetting", (int)state);
            }
            state = STATE_IDLE;
            expected_len = 0U;
            received_len = 0U;
            len_idx = 0U;
            return (esp8266_ipd_frame_t){ .valid = false };
        }

        if (circular_buf_size(rx_cbuf) == 0U) {
            if (timeout_ms == 0U) {
                return (esp8266_ipd_frame_t){ .valid = false };
            }
            Delay_ms(1);
            continue;
        }

        uint8_t byte;
        if (circular_buf_get(rx_cbuf, &byte) != 0) {
            continue;
        }

        // === 调试：打印原始字节流（可选，谨慎使用）===
        // ESP8266_LOG_IPD("RX: 0x%02X ('%c')", byte, isprint(byte) ? byte : '.');

        /* ------------------------ 状态机 ------------------------ */
        switch (state) {
            case STATE_IDLE:
                if (byte == '+') {
                    state = STATE_GOT_PLUS;
                    ESP8266_LOG_IPD("Detected '+'");
                }
                break;

            case STATE_GOT_PLUS:
                state = (byte == 'I') ? STATE_GOT_I : STATE_IDLE;
                if (state == STATE_IDLE) {
                    ESP8266_LOG_IPD("Expected 'I' after '+', got 0x%02X", byte);
                }
                break;

            case STATE_GOT_I:
                state = (byte == 'P') ? STATE_GOT_P : STATE_IDLE;
                if (state == STATE_IDLE) {
                    ESP8266_LOG_IPD("Expected 'P' after '+I', got 0x%02X", byte);
                }
                break;

            case STATE_GOT_P:
                state = (byte == 'D') ? STATE_GOT_D : STATE_IDLE;
                if (state == STATE_IDLE) {
                    ESP8266_LOG_IPD("Expected 'D' after '+IP', got 0x%02X", byte);
                }
                break;

            case STATE_GOT_D:
                state = (byte == ',') ? STATE_AFTER_D : STATE_IDLE;
                if (state == STATE_IDLE) {
                    ESP8266_LOG_IPD("Expected ',' after '+IPD', got 0x%02X", byte);
                }
                break;

            case STATE_AFTER_D:
                if (isdigit(byte)) {
                    len_str[0] = byte;
                    len_idx = 1U;
                    state = STATE_PARSING_LEN;
                    ESP8266_LOG_IPD("Start parsing length, first digit: %c", byte);
                } else {
                    ESP8266_LOG_IPD("Invalid char after '+IPD,', got 0x%02X", byte);
                    state = STATE_IDLE;
                }
                break;

            case STATE_PARSING_LEN:
                if (isdigit(byte) && (len_idx < (sizeof(len_str) - 1U))) {
                    len_str[len_idx++] = byte;
                } else if (byte == ':') {
                    len_str[len_idx] = '\0';
                    expected_len = (uint16_t)atoi(len_str);
                    ESP8266_LOG_IPD("Parsed length: %u", expected_len);

                    if ((expected_len == 0U) || (expected_len > sizeof(data_buf))) {
                        ESP8266_LOG_IPD("Invalid length: %u (max=%u)", expected_len, (uint16_t)sizeof(data_buf));
                        state = STATE_IDLE;
                        expected_len = 0U;
                    } else {
                        received_len = 0U;
                        state = STATE_RECEIVING_DATA;
                    }
                    len_idx = 0U;
                } else if (byte == ',') {
                    // 多连接模式：跳过 conn_id
                    len_idx = 0U;
                    ESP8266_LOG_IPD("Multi-connection mode detected, skipping conn_id");
                } else {
                    ESP8266_LOG_IPD("Unexpected char in length field: 0x%02X", byte);
                    state = STATE_IDLE;
                    len_idx = 0U;
                }
                break;

            case STATE_RECEIVING_DATA:
                if (received_len < sizeof(data_buf)) {
                    data_buf[received_len++] = byte;
                }
                if (received_len >= expected_len) {
					esp8266_ipd_type_t type = classify_ipd_data(data_buf, expected_len);
					
#ifdef ESP8266_DEBUG_IPD     //调试用
					ESP8266_LOG_IPD("Received %u bytes:", expected_len);
					for (uint16_t i = 0; i < expected_len; i++) {
						if (i > 0 && i % 16 == 0) ESP8266_LOG_IPD("");
						Serial_Printf(USART_DEBUG, "%02X ", data_buf[i]);
					}
					Serial_Printf(USART_DEBUG, "\r\n");

					const char* type_str[] = {
						"UNKNOWN", "ACK", "ONENET_CMD", "CUSTOM"
					};
					ESP8266_LOG_IPD("Classified as: %s", 
						(type <= ESP8266_IPD_TYPE_CUSTOM) ? type_str[type] : "INVALID");
#endif

                    // 重置状态机
                    state = STATE_IDLE;
                    expected_len = 0U;
                    received_len = 0U;
                    len_idx = 0U;

                    return (esp8266_ipd_frame_t){
                        .data  = data_buf,
                        .len   = expected_len,
                        .type  = type,
                        .valid = (type != ESP8266_IPD_TYPE_UNKNOWN)
                    };
                }
                break;

            default:
                ESP8266_LOG_IPD("Unknown state %d, resetting", (int)state);
                state = STATE_IDLE;
                len_idx = 0U;
                break;
        }
    }
}
