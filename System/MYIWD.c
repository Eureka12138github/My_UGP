#include "MYIWD.h" 	
/**
 * @brief  Initializes the Independent Watchdog (IWDG) with the specified maximum timeout time.
 * @param  MaxTime: The maximum timeout time in milliseconds.
 *         This parameter must be within the range of 40 to 26214 milliseconds.
 * @retval None
 */
void MYIWD_Init(uint16_t MaxTime) {
    // 初始化重装载时间变量
    float ReloadTime = 0.0f;

    // 预分频系数数组，实际值为 4, 8, 16, 32, 64, 128, 256
    uint8_t prescaler_values[] = {1, 2, 4, 8, 16, 32, 64};

    // 对应的预分频系数枚举值
    uint8_t prescalers[] = {
        IWDG_Prescaler_4, IWDG_Prescaler_8, IWDG_Prescaler_16,
        IWDG_Prescaler_32, IWDG_Prescaler_64, IWDG_Prescaler_128,
        IWDG_Prescaler_256
    };

    // 解除看门狗寄存器的写保护
    IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable);

    // 根据 MaxTime 选择合适的预分频系数
    for (int i = 0; i < 7; i++) {
        // 计算当前预分频系数下的最大时间
        if (MaxTime < 409.6 * prescaler_values[i]) {
            // 设置预分频系数
            IWDG_SetPrescaler(prescalers[i]);
            // 计算重装载值
            // 公式: ReloadTime = MaxTime / (TLSI * PR) - 1
            // 其中 TLSI = 0.025（1/40）, PR = prescaler_values[i] * 4
            ReloadTime = MaxTime / 0.025 / (prescaler_values[i] * 4) - 1;
            break;
        }
    }

    // 检查重装载值是否在有效范围内
    if (ReloadTime <= 4096) {
        // 设置重装载值
        IWDG_SetReload((u16)ReloadTime);
    }

    // 手动喂狗，重新加载计数器
    IWDG_ReloadCounter();//这是喂狗代码，应在main函数的while(1)循环的合适位置放置，确保及时喂狗

    // 启动看门狗
    IWDG_Enable();
}
/**
 * @brief  Displays a reset message on the OLED screen.
 * @param  message: The message to display.
 * @retval None
 */
void Display_Reset_Message(char* message,char* errortype) {
    // 显示消息
    OLED_ShowChinese(66, 0, message, OLED_12X12_FULL);
	OLED_ShowChinese(66, 18, errortype, OLED_12X12_FULL);
    OLED_Update();
    Delay_ms(500);
    // 清除消息
	OLED_ClearArea(66,0,127-65,32);
	OLED_Update();
    Delay_ms(200);
    // 再次显示消息
    OLED_ShowChinese(66, 0, message, OLED_12X12_FULL);
	OLED_ShowChinese(66, 18, errortype, OLED_12X12_FULL);
    OLED_Update();
    Delay_ms(500);
    // 再次清除消息
	OLED_ClearArea(66,0,127-65,32);
	OLED_Update();
    Delay_ms(200);
	
}
/**
 * @brief  Checks the reset reason and displays the appropriate message on the OLED screen.
 * @retval None
 */
void Check_Reset_Way(void) {
	Reset_Count = Store_Data[RESET_TIMERS_Store_IDX];//现将原来已储存的复位次数取出
    // 检查复位标志是否由看门狗产生
    if (RCC_GetFlagStatus(RCC_FLAG_IWDGRST) == SET) {
        // 显示图标
        OLED_ShowImageArea(0, 0, 64, 64, 0, 0, 64, 64, USC_LOGO_64);
        // 更新复位计数
        Reset_Count++;
        Store_Data[RESET_TIMERS_Store_IDX] = Reset_Count;
        Store_Save();
		for(u8 i =0;i < ERROR_TIME_ARRAY_SIZE;i++){   
			if(ErrorTime[i].errortype != 0){
				if(ErrorTime[i].errortype == ENV_COMM_DATA_TRANSMISSION_FAILURE){
					Display_Reset_Message("看门狗复位","无法发送！");
				}else if(ErrorTime[i].errortype == ENV_COMM_DATA_RECEPTION_FAILURE){
						Display_Reset_Message("看门狗复位","无法接收！");
				}else if(ErrorTime[i].errortype == ENV_SENSOR_DUST_ANOMALY){
						Display_Reset_Message("看门狗复位","扬尘异常！");
				}else if(ErrorTime[i].errortype == ENV_SENSOR_NOISE_ANOMALY){
						Display_Reset_Message("看门狗复位","噪音异常！");
				}
				break;
			}
		}
		if(ErrorTime[0].errortype == 0 && ErrorTime[1].errortype == 0 && ErrorTime[2].errortype == 0){
		Display_Reset_Message("看门狗复位","未知错误");
		}
        // 显示“看门狗复位”信息
        
        // 清除复位标志位
        RCC_ClearFlag();
    } else {
        // 显示图标
        OLED_ShowImageArea(0, 0, 64, 64, 0, 0, 64, 64, USC_LOGO_64);
        // 显示“硬件复位”信息
        Display_Reset_Message("硬件复位"," ");//空格为占位
    }
}


/*如果使用了独立看门狗复位，必须在main函数之中，while(1)循环之前加上下面的if判断并清除标志位，只有两句便不封装了。
	if(RCC_GetFlagStatus(RCC_FLAG_IWDGRST) == SET)//如果复位是由看门狗产生的
	{
	
		RCC_ClearFlag();//清除标志位，该标志位必须软件清除！
	}

*/
