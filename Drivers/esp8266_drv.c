/**
 ************************************************************
 * 文件名：    esp8266_drv.c
 * 作者：      Eureka
 * 日期：      2026-01-29
 * 说明：      ESP8266 驱动（基于环形缓冲区串口，无全局接收缓存）
 ************************************************************
 */
 
#include "esp8266_drv.h"// Device header

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
 ******************************************************************************
 * @brief  初始化 ESP8266 模块（AT 指令握手 + 基础配置）
 *
 * @details 执行以下初始化步骤：
 *          1. 发送 "AT" 测试通信是否正常
 *          2. 设置工作模式为 STA（AT+CWMODE=1）
 *          3. 启用 DHCP 自动获取 IP（AT+CWDHCP=1,1）
 *          4. 连接预设的 WiFi 网络（通过 WIFI_CONNECT_CMD 宏定义）
 *
 * @retval 0: 成功完成全部初始化
 * @retval 1: AT 指令无响应（通信失败）
 * @retval 2: STA 模式设置失败
 * @retval 3: DHCP 启用失败
 * @retval 4: WiFi 连接失败（未收到 "GOT IP"）
 *
 * @note   - 本函数依赖以下前提已满足：
 *           • 已调用 Delay_Init()（SysTick 正常工作）
 *           • 已调用 ESP8266_HardwareInit()（串口与复位引脚已初始化）
 *           • WIFI_CONNECT_CMD 宏已在头文件中正确定义（如 "AT+CWJAP=\"SSID\",\"PWD\"\r\n"）
 *
 * @warning 调试日志通过条件编译控制，默认关闭。若需查看初始化流程，请按以下步骤操作：
 *          1. 打开头文件 `debug_config.h`
 *          2. 取消注释以下宏定义：
 *             - `#define ESP8266_DEBUG_INIT`  → 显示初始化阶段日志（推荐开启）
 *          3. 确保 `USART_DEBUG` 已被修改为调试串口用于输出
 *          4. OLED 状态提示始终生效，用于无串口场景的用户反馈
 *
 *          ⚠️ 注意：调试日志仅用于开发阶段！发布固件前请务必关闭所有 ESP8266_DEBUG_INIT* 宏，
 *                   以避免占用 Flash 空间、泄露信息或影响性能。
 ******************************************************************************
 */
u8 ESP8266_Init(void)
{
    const u8 maxRetries = 3;
    u8 retryCount;

    ESP8266_LOG_INIT("=== ESP8266 初始化开始 ===");

    /* 清除 OLED 状态区（准备显示新状态） */
    OLED_ClearArea(66, 32, 50, 16);

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
        OLED_ShowString(66, 32, "AT ERR", OLED_7X12_HALF);
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
        OLED_ShowString(66, 32, "STA ERR", OLED_7X12_HALF);
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
        OLED_ShowString(66, 32, "DHCP ERR", OLED_7X12_HALF);
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
        OLED_ShowString(66, 32, "WIFI ERR", OLED_7X12_HALF);
        return 4;
    }

    /* 全部成功：可选择显示 OK 或保持空白 */
    // OLED_ShowString(66, 32, "OK", OLED_7X12_HALF); // 可选
    ESP8266_LOG_INIT("=== ESP8266 初始化完成！===");
    return 0;
}
