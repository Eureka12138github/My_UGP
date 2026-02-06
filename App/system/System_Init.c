#include "System_Init.h"
#include "bsp_pms7003.h"



/**
 * @brief  初始化硬件。
 * @retval None
 */
void Initialize_Hardware(void) {
    OLED_Init();//OLED屏初始化，与数据显示有关
    Timer2_Init();//定时2初始化，与任务调度有关
    Store_Init();//FLASH初始化，便于后续存储阈值与独立看门狗次数
    Alarm_Init();//警报初始化
	BSP_PMS7003_Init();//扬尘传感器初始化
	BSP_XM7903_Init();//噪音传感器初始化
	ESP8266_HardwareInit();
	Usart3_Init(9600);   
	
}


/**
 * @brief  初始化系统。
 * @retval None
 */
void Initialize_System(void) {
    u8 retry_count = 0;
    const u8 MAX_RETRY_COUNT = 5; // 最大重试次数
    
    /* *************************************************************************** *\
    *                           系统启动画面显示
    \* *************************************************************************** */
    // 显示启动Logo图标
    OLED_Clear();
    OLED_ShowImageArea(0, 0, 64, 64, 0, 0, 64, 64, USC_LOGO_64);
    OLED_Update();

    /* *************************************************************************** *\
    *                           DHT11传感器初始化
    \* *************************************************************************** */
    // 检测DHT11传感器存在性
    retry_count = 0;
    while(!DHT11_Init()) {
        retry_count++;
        OLED_ShowChinese(66, 0, "未找到", OLED_12X12_FULL);
        OLED_ShowString(66, 16, "DHT11!", OLED_7X12_HALF);
        OLED_ShowChinese(66, 48, "重试：", OLED_12X12_FULL);
        OLED_ShowNum(102, 48, retry_count, 2, OLED_7X12_HALF);
        OLED_UpdateArea(66, 0, 127-65, 63);
        
        if(retry_count >= MAX_RETRY_COUNT) {
            OLED_ClearArea(66, 0, 127-65, 63);
            OLED_ShowString(66, 0, "Skip DHT11", OLED_7X12_HALF);
            OLED_ShowString(66, 16, "Continue...", OLED_7X12_HALF);
            OLED_Update();
            Delay_s(3);
            break; // 跳过DHT11初始化，继续后续流程
        }
        Delay_ms(1000);
    }    

    OLED_ClearArea(66, 0, 127-65, 63);
    
    /* *************************************************************************** *\
    *                           ESP8266 WiFi模块初始化
    \* *************************************************************************** */
    // 初始化ESP8266 WiFi模块
    OLED_ShowString(66, 0, "esp8266", OLED_7X12_HALF);
    OLED_ShowChinese(66, 16, "初始化中", OLED_12X12_FULL);
    OLED_Update();
    
    retry_count = 0;
    while(ESP8266_Init()) {
        retry_count++;
        OLED_ShowChinese(66, 16, "初始化失败", OLED_12X12_FULL);
        OLED_ShowChinese(66, 48, "重试：", OLED_12X12_FULL);
        OLED_ShowNum(102, 48, retry_count, 2, OLED_7X12_HALF);
        OLED_Update();
        
        if(retry_count >= MAX_RETRY_COUNT) {
            OLED_ClearArea(66, 0, 127-65, 63);
            OLED_ShowString(66, 0, "ESP8266", OLED_7X12_HALF);
            OLED_ShowChinese(66, 16, "初始化失败", OLED_12X12_FULL);
            OLED_ShowChinese(66, 32, "请检查连接", OLED_12X12_FULL);
            OLED_ShowChinese(66, 48, "或网络状态", OLED_12X12_FULL);
            OLED_Update();
            Delay_s(3);
            NVIC_SystemReset(); // ESP8266初始化失败，系统复位重试
        }
        Delay_ms(2000);
    }
        
    /* *************************************************************************** *\
    *                           OneNET平台TCP连接
    \* *************************************************************************** */
    // 建立与OneNET平台的TCP连接
    OLED_ClearArea(66, 0, 127-65, 63);
    OLED_ShowChinese(66, 0, "正在接入", OLED_12X12_FULL);
    OLED_ShowString(66, 16, "OneNET", OLED_7X12_HALF);    
    OLED_Update();    
    
    retry_count = 0;
    while (ESP8266_SendCmd(ESP8266_ONENET_INFO, "CONNECT")) {
        retry_count++;
        if(retry_count >= MAX_RETRY_COUNT) {
            OLED_ClearArea(66, 0, 127-65, 63);
            OLED_ShowChinese(66, 0, "连接失败", OLED_12X12_FULL);
            OLED_ShowChinese(66, 16, "请检查网络", OLED_12X12_FULL);
            OLED_ShowChinese(66, 32, "即将重试", OLED_12X12_FULL);
            OLED_Update();
            Delay_s(3);
            NVIC_SystemReset(); // TCP连接失败，系统复位重试
        }
        Delay_ms(2000);
    }

    /* *************************************************************************** *\
    *                           OneNET平台MQTT连接
    \* *************************************************************************** */
    // 建立与OneNET平台的MQTT连接
    retry_count = 0; 
    while (retry_count < MAX_RETRY_COUNT) {			
        unsigned char errorCode = OneNet_DevLink(); 		
        if (errorCode == 0) {
            // 连接成功，跳出重试循环
            break;
        }

        // 显示当前重试信息
        retry_count++;
        OLED_ClearArea(66, 0, 127 - 65, 63);
        OLED_ShowChinese(66, 0, "设备连接", OLED_12X12_FULL);
        OLED_ShowChinese(66, 16, "失败：", OLED_12X12_FULL);
        OLED_ShowNum(104, 16, errorCode, 2, OLED_7X12_HALF);
        OLED_ShowChinese(66, 32, "重试次数:", OLED_12X12_FULL);
        OLED_ShowNum(66, 48, retry_count, 2, OLED_7X12_HALF);
        OLED_Update();

        // 判断是否为不可恢复的错误（无需重试）
        if (errorCode == 1 || errorCode == 2 || errorCode == 4 || errorCode == 5 || 
            errorCode == 6 || errorCode == 7 || errorCode == 9 || errorCode == 10) {
            // 不可恢复错误：鉴权失败、MQTT包构造失败、非CONNACK包、数据包格式错误、
            // 协议版本不可接受、客户端标识符被拒绝、用户名或密码错误、未授权连接
            OLED_ClearArea(66, 0, 127 - 65, 63);
            OLED_ShowChinese(66, 0, "配置错误", OLED_12X12_FULL);
            OLED_ShowChinese(66, 16, "请检查设备", OLED_12X12_FULL);
            OLED_ShowChinese(66, 32, "参数!", OLED_12X12_FULL);
            OLED_Update();
            Delay_s(3);
            NVIC_SystemReset(); // 配置错误，系统复位
        }

        // 可恢复错误（超时或通信失败），继续重试
        if (retry_count >= MAX_RETRY_COUNT) {
            // 重试耗尽
            OLED_ClearArea(66, 0, 127 - 65, 63);
            OLED_ShowChinese(66, 0, "重试耗尽", OLED_12X12_FULL);
            OLED_ShowChinese(66, 16, "请检查网络", OLED_12X12_FULL);
            OLED_ShowChinese(66, 32, "即将重启", OLED_12X12_FULL);
            OLED_Update();
            Delay_ms(3000);
            NVIC_SystemReset(); // 重试次数用尽，系统复位
        }

        Delay_ms(3000); // 重试间隔
    }
    
    /* *************************************************************************** *\
    *                           OneNET主题订阅及初始化完成提示
    \* *************************************************************************** */ 
	// 订阅OneNET平台主题
    OneNET_Subscribe();
    // 显示初始化完成提示
    OLED_ClearArea(66, 0, 127-65, 63);
    OLED_ShowChinese(66, 0, "连接成功！", OLED_12X12_FULL);
    OLED_ShowChinese(66, 16, "网络初始化", OLED_12X12_FULL);
    OLED_ShowChinese(66, 32, "完毕！即将", OLED_12X12_FULL);
    OLED_ShowChinese(66, 48, "进入系统", OLED_12X12_FULL);
    OLED_Update();  
    Delay_ms(1000);	
    OLED_Clear();  	
    
    /* *************************************************************************** *\
    *                           系统参数初始化
    \* *************************************************************************** */
    // 初始化环境监测阈值参数
    Dust_Limit = (Store_Data[DUST_LIMIT_STORE_IDX] != 0) ? Store_Data[DUST_LIMIT_STORE_IDX] : Default_Dust_Limit;
    Noise_Limit = (Store_Data[NOISE_LIMIT_STORE_IDX] != 0) ? Store_Data[NOISE_LIMIT_STORE_IDX] : Default_Noise_Limit;
    
	ReadStoreErrorTime();
}
