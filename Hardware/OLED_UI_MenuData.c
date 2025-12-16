#include "OLED_UI_MenuData.h"
#include "OLED_UI.h"

/*此文件用于存放菜单数据。实际上菜单数据可以存放在任何地方，存放于此处是为了规范与代码模块化*/

// ColorMode 是一个在OLED_UI当中定义的bool类型变量，用于控制OLED显示的颜色模式， DARKMODE 为深色模式， LIGHTMOOD 为浅色模式。这里将其引出是为了创建单选框菜单项。
extern bool ColorMode;
extern bool Limit_Save;
extern bool Clear_Data;
extern bool Alarm_Off_Manual;
// OLED_UI_Brightness 是一个在OLED_UI当中定义的int16_t类型变量，用于控制OLED显示的亮度。这里将其引出是为了创建调整亮度的滑动条窗口，范围0-255。
extern u16 OLED_UI_Brightness;
float testfloatnum = 0.5;
u16 Dust_Limit = Default_Dust_Limit;
u16 Noise_Limit = Default_Noise_Limit;
u16 Reset_Count = 0;
u16 decibels;
u16 temp;
u16 humi;
u16 PM2_5_ENV;
u8 Error1 = 1;
u8 Error2 = 2;
u8 Error3 = 3;


PM_SensorData PM_Data = {0};
#define SPEED 10

MenuWindow LimitSaveWindow = {
	.General_Width = 75,								//窗口宽度
	.General_Height = 18, 							//窗口高度
	.General_WindowType = WINDOW_ROUNDRECTANGLE, 	//窗口类型
	.General_ContinueTime = 3.0,						//窗口持续时间

	.Text_String = "阈值已保存^_^",					//窗口标题
	.Text_FontSize = OLED_UI_FONT_12,				//字高
	.Text_FontSideDistance = 8,							//字体距离左侧的距离
	.Text_FontTopDistance = 2,							//字体距离顶部的距离
	

};
MenuWindow ClearDataWindow = {
	.General_Width = 75,								//窗口宽度
	.General_Height = 18, 							//窗口高度
	.General_WindowType = WINDOW_ROUNDRECTANGLE, 	//窗口类型
	.General_ContinueTime = 4.0,						//窗口持续时间

	.Text_String = "已清除数据^_^",					//窗口标题
	.Text_FontSize = OLED_UI_FONT_12,				//字高
	.Text_FontSideDistance = 8,							//字体距离左侧的距离
	.Text_FontTopDistance = 2,							//字体距离顶部的距离
	

};

//关于窗口的结构体
MenuWindow SetBrightnessWindow = {
	.General_Width = 80,								//窗口宽度
	.General_Height = 28, 							//窗口高度
	.Text_String = "屏幕亮度",					//窗口标题
	.Text_FontSize = OLED_UI_FONT_12,				//字高
	.Text_FontSideDistance = 4,							//字体距离左侧的距离
	.Text_FontTopDistance = 3,							//字体距离顶部的距离
	.General_WindowType = WINDOW_ROUNDRECTANGLE, 	//窗口类型
	.General_ContinueTime = 4.0,						//窗口持续时间

	.Prob_Data_u16 = &OLED_UI_Brightness,				//显示的变量地址                   
	.Prob_DataStep = 5,								//步长
	.Prob_MinData = 5,									//最小值
	.Prob_MaxData = 255, 								//最大值
	.Prob_BottomDistance = 3,							//底部间距
	.Prob_LineHeight = 8,								//进度条高度
	.Prob_SideDistance = 4,								//边距
};
/**
 * @brief 创建显示亮度窗口
 */
void BrightnessWindow(void){
	OLED_UI_CreateWindow(&SetBrightnessWindow);
}
void ShowLimitSavedWindow(void){
	OLED_UI_CreateWindow(&LimitSaveWindow);
}
void ShowClearDataWindow(void){
	OLED_UI_CreateWindow(&ClearDataWindow);
}

MenuWindow Dust_Limit_Window = {
	.General_Width = 80,								//窗口宽度
	.General_Height = 28, 								//窗口高度
	.General_WindowFor = DUST_LIMIT_Store_IDX,			//窗口用途
	.Text_String = "PM2.5阈值(ug/m3)",					//窗口标题
	.Text_FontSize = OLED_UI_FONT_12,					//字高
	.Text_FontSideDistance = 4,							//字体距离左侧的距离
	.Text_FontTopDistance = 3,							//字体距离顶部的距离
	.General_WindowType = WINDOW_ROUNDRECTANGLE, 		//窗口类型
	.General_ContinueTime = 4.0,						//窗口持续时间

	.Prob_Data_u16 = &Dust_Limit,						//显示的变量地址
	.Prob_DataStep = 1,									//步长
	.Prob_MinData = 0,									//最小值
	.Prob_MaxData = 200, 								//最大值
	.Prob_BottomDistance = 3,							//底部间距
	.Prob_LineHeight = 8,								//进度条高度
	.Prob_SideDistance = 4,	
};

MenuWindow Noise_Limit_Window = {
	.General_Width = 80,								//窗口宽度
	.General_Height = 28, 								//窗口高度
	.General_WindowFor = NOISE_LIMIT_Store_IDX,				//窗口用途
	.Text_String = "噪音阈值(dBA)",						//窗口标题
	.Text_FontSize = OLED_UI_FONT_12,					//字高
	.Text_FontSideDistance = 4,							//字体距离左侧的距离
	.Text_FontTopDistance = 3,							//字体距离顶部的距离
	.General_WindowType = WINDOW_ROUNDRECTANGLE, 		//窗口类型
	.General_ContinueTime = 4.0,						//窗口持续时间

	.Prob_Data_u16 = &Noise_Limit,						//显示的变量地址
	.Prob_DataStep = 1,									//步长
	.Prob_MinData = 0,									//最小值
	.Prob_MaxData = 100, 								//最大值
	.Prob_BottomDistance = 3,							//底部间距
	.Prob_LineHeight = 8,								//进度条高度
	.Prob_SideDistance = 4,	
};

/**
 * @brief 创建显示窗口
 */

void ShowDust_Limit_Window(void){
	OLED_UI_CreateWindow(&Dust_Limit_Window);
}

void ShowNoise_Limit_Window(void){
	OLED_UI_CreateWindow(&Noise_Limit_Window);
}

//主LOGO移动的结构体
OLED_ChangePoint LogoMove;
//主LOGO文字移动的结构体
OLED_ChangePoint LogoTextMove;
//welcome文字移动的结构体
OLED_ChangePoint WelcomeTextMove;

extern OLED_ChangePoint OLED_UI_PageStartPoint ;



//设置菜单项的辅助显示函数
void SettingAuxFunc(void){
	//在规定位置显示LOGO
	if(fabs(OLED_UI_PageStartPoint.CurrentPoint.X - OLED_UI_PageStartPoint.TargetPoint.X) < 4){
		LogoMove.TargetPoint.X = 0;
		LogoMove.TargetPoint.Y = 0;
	}
	//将LOGOTEXT移动到屏幕右侧看不见的地方
	LogoTextMove.TargetPoint.X = 129;
	LogoTextMove.TargetPoint.Y = 0;
	//将Welcome文字移动到屏幕底部看不见的地方
	WelcomeTextMove.TargetPoint.X = 128;
	WelcomeTextMove.TargetPoint.Y = 0;
	ChangePoint(&LogoMove);
	OLED_ShowImageArea(LogoMove.CurrentPoint.X,LogoMove.CurrentPoint.Y,32,64,0,0,128,128,OLED_UI_SettingsLogo);
	ChangePoint(&LogoTextMove);
	OLED_ShowImageArea(LogoTextMove.CurrentPoint.X,LogoTextMove.CurrentPoint.Y,26,64,0,0,128,128,OLED_UI_LOGOTEXT64);
	ChangePoint(&WelcomeTextMove);
	OLED_ShowImageArea(WelcomeTextMove.CurrentPoint.X,WelcomeTextMove.CurrentPoint.Y,16,64,0,0,128,128,OLED_UI_LOGOGithub);
}

//关于菜单的辅助显示函数
void AboutThisDeviceAuxFunc(void){
	//将LOGO移动到屏幕上方看不见的地方
	LogoMove.TargetPoint.X = 0;
	LogoMove.TargetPoint.Y = -80;
	ChangePoint(&LogoMove);
    OLED_ShowImageArea(LogoMove.CurrentPoint.X,LogoMove.CurrentPoint.Y,32,64,0,0,128,128,OLED_UI_SettingsLogo);
	//在屏幕右侧显示LOGO文字
	if(fabs(OLED_UI_PageStartPoint.CurrentPoint.X - OLED_UI_PageStartPoint.TargetPoint.X) < 4){
		LogoTextMove.TargetPoint.X = 102;
		LogoTextMove.TargetPoint.Y = 0;
	}
	ChangePoint(&LogoTextMove);
	OLED_ShowImageArea(LogoTextMove.CurrentPoint.X,LogoTextMove.CurrentPoint.Y,26,64,0,0,128,128,OLED_UI_LOGOTEXT64);
}
//关于OLED UI的辅助显示函数
void AboutOLED_UIAuxFunc(void){
	//将LOGO移动到屏幕上方看不见的地方
	LogoMove.TargetPoint.X = 0;
	LogoMove.TargetPoint.Y = -80;
	ChangePoint(&LogoMove);
	OLED_ShowImageArea(LogoMove.CurrentPoint.X,LogoMove.CurrentPoint.Y,32,64,0,0,128,128,OLED_UI_SettingsLogo);
	//在屏幕右测显示Welcome文字
	if(fabs(OLED_UI_PageStartPoint.CurrentPoint.X - OLED_UI_PageStartPoint.TargetPoint.X) < 4){
		WelcomeTextMove.TargetPoint.X = 110;
		WelcomeTextMove.TargetPoint.Y = 0;
	}
	ChangePoint(&WelcomeTextMove);
	OLED_ShowImageArea(WelcomeTextMove.CurrentPoint.X,WelcomeTextMove.CurrentPoint.Y,16,64,0,0,128,128,OLED_UI_LOGOGithub);

}
//主菜单的辅助显示函数
void MainAuxFunc(void){
	//不显示
	LogoMove.TargetPoint.X = -200;
	LogoMove.TargetPoint.Y = 0;
	LogoMove.CurrentPoint.X = -200;
	LogoMove.CurrentPoint.Y = 0;

	LogoTextMove.TargetPoint.X = 129;
	LogoTextMove.TargetPoint.Y = 0;
	LogoTextMove.CurrentPoint.X = 129;
	LogoTextMove.CurrentPoint.Y = 0;
	
	WelcomeTextMove.TargetPoint.X = 128;
	WelcomeTextMove.TargetPoint.Y = 0;
	WelcomeTextMove.CurrentPoint.X = 128;
	WelcomeTextMove.CurrentPoint.Y = 0;
}

//主菜单的菜单项
MenuItem MainMenuItems[] = {

	{.General_item_text = "监控台",.General_callback = NULL,.General_SubMenuPage = &Monitor_Station_MenuPage,.Tiles_Icon = Monitor_Station},
	{.General_item_text = "设置",.General_callback = NULL,.General_SubMenuPage = &SettingsMenuPage,.Tiles_Icon = My_Settings},
	{.General_item_text = "更多",.General_callback = NULL,.General_SubMenuPage = &MoreMenuPage,.Tiles_Icon = My_Image_more},   	
	{.General_item_text = NULL},/*最后一项的General_item_text置为NULL，表示该项为分割线*/

};

//监控台的菜单项
MenuItem Monitor_Station_MenuItems[] = {

	{.General_item_text = "PM2.5:%dug/m3",
	.General_callback = NULL,
	.General_SubMenuPage = NULL,
	.List_BoolRadioBox = NULL,
	.General_Value = &PM_Data.pm2_5_env
	},
	
	{.General_item_text = "噪音:%ddBA",
	.General_callback = NULL,
	.General_SubMenuPage = NULL,
	.List_BoolRadioBox = NULL,
	.General_Value = &decibels	
	},
	
	{.General_item_text = "PM2.5阈值:%dug/m3",
	.General_callback = ShowDust_Limit_Window,
	.General_SubMenuPage = NULL,
	.List_BoolRadioBox = NULL,
	.General_Value = &Dust_Limit,	
	},
	
	{.General_item_text = "噪音阈值:%ddBA",
	.General_callback = ShowNoise_Limit_Window,
	.General_SubMenuPage = NULL,
	.List_BoolRadioBox = NULL,
	.General_Value = &Noise_Limit,	
	},
	
	{.General_item_text = "温度:%d`C",
	.General_callback = NULL,
	.General_SubMenuPage = NULL,
	.List_BoolRadioBox = NULL,
	.General_Value = &temp,	
	},
	{.General_item_text = "湿度:%d%%RH",
	.General_callback = NULL,
	.General_SubMenuPage = NULL,
	.List_BoolRadioBox = NULL,
	.General_Value = &humi,	
	},
	
	{.General_item_text = "更多扬尘数据",
	.General_callback = NULL,
	.General_SubMenuPage = &MoreDustDataPage,
	.List_BoolRadioBox = NULL
	},
	
	{.General_item_text = "阈值保存",
	.General_callback = ShowLimitSavedWindow,
	.General_SubMenuPage = NULL,
	.List_BoolRadioBox = &Limit_Save
	},	
	
	{.General_item_text = "关闭警报",
	.General_callback = NULL,
	.General_SubMenuPage = NULL,
	.List_BoolRadioBox = &Alarm_Off_Manual
	},
	
	{.General_item_text = "ReSet Times:%d",
	.General_callback = NULL,
	.General_SubMenuPage = NULL,
	.List_BoolRadioBox = NULL,
	.General_Value = &Reset_Count,	
	},	
	
	{.General_item_text = "[返回]",
	.General_callback = OLED_UI_Back,
	.General_SubMenuPage = NULL,
	.List_BoolRadioBox = NULL
	},	
	{.General_item_text = NULL},/*最后一项的General_item_text置为NULL，表示该项为分割线*/

};
//设置菜单项内容数组
MenuItem SettingsMenuItems[] = {
	{.General_item_text = "亮度",.General_callback = BrightnessWindow,.General_SubMenuPage = NULL,.List_BoolRadioBox = NULL},
	{.General_item_text = "黑暗模式",.General_callback = NULL,.General_SubMenuPage = NULL,.List_BoolRadioBox = &ColorMode},
	{.General_item_text = "清除数据",.General_callback = ShowClearDataWindow,.General_SubMenuPage = NULL,.List_BoolRadioBox = &Clear_Data},	
	{.General_item_text = "[返回]",.General_callback = OLED_UI_Back,.General_SubMenuPage = NULL,.List_BoolRadioBox = NULL},
	{.General_item_text = NULL},/*最后一项的General_item_text置为NULL，表示该项为分割线*/
};          


MenuItem AboutThisSystemuItems[] = {
	{.General_item_text = "-[Name:]",.General_callback = NULL,.General_SubMenuPage = NULL,.List_BoolRadioBox = NULL},
	{.General_item_text = "IoT-based Real-Time Monitoring System for Construction Site Dust and Noise",.General_callback = NULL,.General_SubMenuPage = NULL,.List_BoolRadioBox = NULL},	
	{.General_item_text = "-[Author:]",.General_callback = NULL,.General_SubMenuPage = NULL,.List_BoolRadioBox = NULL},
	{.General_item_text = " Eureka",.General_callback = NULL,.General_SubMenuPage = NULL,.List_BoolRadioBox = NULL},
	{.General_item_text = "-[Adress:]",.General_callback = NULL,.General_SubMenuPage = NULL,.List_BoolRadioBox = NULL},
	{.General_item_text = " https://github.com/Eureka12138github/OLEDMENU",.General_callback = NULL,.General_SubMenuPage = NULL,.List_BoolRadioBox = NULL},
	{.General_item_text = "-[MCU:]",.General_callback = NULL,.General_SubMenuPage = NULL,.List_BoolRadioBox = NULL},
	{.General_item_text = " STM32F103C8T6",.General_callback = NULL,.General_SubMenuPage = NULL,.List_BoolRadioBox = NULL},
	{.General_item_text = " RAM:20KB",.General_callback = NULL,.General_SubMenuPage = NULL,.List_BoolRadioBox = NULL},
	{.General_item_text = " ROM:64KB",.General_callback = NULL,.General_SubMenuPage = NULL,.List_BoolRadioBox = NULL},
	{.General_item_text = "-[Screen:]",.General_callback = NULL,.General_SubMenuPage = NULL,.List_BoolRadioBox = NULL},
	{.General_item_text = " SSD1306 128x64 OLED",.General_callback = NULL,.General_SubMenuPage = NULL,.List_BoolRadioBox = NULL},
	{.General_item_text = " SoftWare I2C",.General_callback = NULL,.General_SubMenuPage = NULL,.List_BoolRadioBox = NULL},	
	{.General_item_text = "-[Sensors:]",.General_callback = NULL,.General_SubMenuPage = NULL,.List_BoolRadioBox = NULL},
	{.General_item_text = " PMS7003",.General_callback = NULL,.General_SubMenuPage = NULL,.List_BoolRadioBox = NULL},	
	{.General_item_text = " XM7903T",.General_callback = NULL,.General_SubMenuPage = NULL,.List_BoolRadioBox = NULL},
	{.General_item_text = " DHT11",.General_callback = NULL,.General_SubMenuPage = NULL,.List_BoolRadioBox = NULL},	
	{.General_item_text = "-[CM:]",.General_callback = NULL,.General_SubMenuPage = NULL,.List_BoolRadioBox = NULL},	
	{.General_item_text = " ESP8266 WIFI",.General_callback = NULL,.General_SubMenuPage = NULL,.List_BoolRadioBox = NULL},	
	{.General_item_text = "-[CP:]",.General_callback = NULL,.General_SubMenuPage = NULL,.List_BoolRadioBox = NULL},
	{.General_item_text = " MQTT",.General_callback = NULL,.General_SubMenuPage = NULL,.List_BoolRadioBox = NULL},		
	{.General_item_text = "-[IOT Platform:]",.General_callback = NULL,.General_SubMenuPage = NULL,.List_BoolRadioBox = NULL},
	{.General_item_text = " OneNet",.General_callback = NULL,.General_SubMenuPage = NULL,.List_BoolRadioBox = NULL},		
	{.General_item_text = "[返回]",.General_callback = OLED_UI_Back,.General_SubMenuPage = NULL,.List_BoolRadioBox = NULL},
	{.General_item_text = NULL},/*最后一项的General_item_text置为NULL，表示该项为分割线*/
};

MenuItem MoreMenuItems[] = {                         
	
	{.General_item_text = "错误日志",.General_callback = NULL,.General_SubMenuPage = &ErrorMenuPage,.List_BoolRadioBox = NULL},
	{.General_item_text = "警报日志",.General_callback = NULL,.General_SubMenuPage = &WarningMenuPage,.List_BoolRadioBox = NULL},
//	{.General_item_text = "小菜单",.General_callback = NULL,.General_SubMenuPage = &SmallAreaMenuPage,.List_BoolRadioBox = NULL},
	{.General_item_text = "此系统",.General_callback = NULL,.General_SubMenuPage = &AboutThisSystemPage,.List_BoolRadioBox = NULL},	
	{.General_item_text = "时间：%d年%d月%d日%d:%d:%d",.General_callback = NULL,.General_SubMenuPage = NULL,.TimeValue = &Time},
	{.General_item_text = "[返回]",.General_callback = OLED_UI_Back,.General_SubMenuPage = NULL,.List_BoolRadioBox = NULL},	
	{.General_item_text = NULL},/*最后一项的General_item_text置为NULL，表示该项为分割线*/
};

MenuItem ErrorMenuItems[] = {
	
	{
	.General_item_text = "错误类型:%d(%d年%d月%d日%d:%d:%d)",
	.ShowErrorMeg = &ErrorTime[0].errorshowflag,
	.General_Value1 = &ErrorTime[0].errortype,
	.TimeValue = &ErrorTime[0].errortime
	},
	
	{
	.General_item_text = "错误类型:%d(%d年%d月%d日%d:%d:%d)",
	.ShowErrorMeg = &ErrorTime[1].errorshowflag,
	.General_Value1 = &ErrorTime[1].errortype,
	.TimeValue = &ErrorTime[1].errortime
	},
	
	{
	.General_item_text = "错误类型:%d(%d年%d月%d日%d:%d:%d)",
	.ShowErrorMeg = &ErrorTime[2].errorshowflag,
	.General_Value1 = &ErrorTime[2].errortype,
	.TimeValue = &ErrorTime[2].errortime
	},
	
	{.General_item_text = "类型说明",.General_callback = NULL,.General_SubMenuPage = &ErrorTypeExplanationPage},
	{.General_item_text = "[返回]",.General_callback = OLED_UI_Back,.General_SubMenuPage = NULL,.List_BoolRadioBox = NULL},
	{.General_item_text = NULL},/*最后一项的General_item_text置为NULL，表示该项为分割线*/
};

MenuItem WarningMenuItems[] = {
	
	{
	.General_item_text = "警报类型:%d(%d年%d月%d日%d:%d:%d)",
	.ShowErrorMeg = &WarningTime[0].warningshowflag,
	.General_Value1 = &WarningTime[0].warningtype,
	.TimeValue = &WarningTime[0].warningtime
	},
	
	{
	.General_item_text = "警报类型:%d(%d年%d月%d日%d:%d:%d)",
	.ShowErrorMeg = &WarningTime[1].warningshowflag,
	.General_Value1 = &WarningTime[1].warningtype,
	.TimeValue = &WarningTime[1].warningtime
	},
	
	{
	.General_item_text = "警报类型:%d(%d年%d月%d日%d:%d:%d)",
	.ShowErrorMeg = &WarningTime[2].warningshowflag,
	.General_Value1 = &WarningTime[2].warningtype,
	.TimeValue = &WarningTime[2].warningtime
	},
	{.General_item_text = "类型说明",.General_callback = NULL,.General_SubMenuPage = &WarningTypeExplanationPage},	
	{.General_item_text = "[返回]",.General_callback = OLED_UI_Back,.General_SubMenuPage = NULL,.List_BoolRadioBox = NULL},
	{.General_item_text = NULL},/*最后一项的General_item_text置为NULL，表示该项为分割线*/
};

MenuItem MoreDustDataItems[] = {
	{.General_item_text = "PM1.0_Cf1:%d",.General_callback = NULL,.General_SubMenuPage = NULL,.List_BoolRadioBox = NULL,.General_Value = &PM_Data.pm1_0_cf1},
	{.General_item_text = "PM2.5_Cf1:%d",.General_callback = NULL,.General_SubMenuPage = NULL,.List_BoolRadioBox = NULL,.General_Value = &PM_Data.pm2_5_cf1},
	{.General_item_text = "PM10_Cf1:%d",.General_callback = NULL,.General_SubMenuPage = NULL,.List_BoolRadioBox = NULL,.General_Value = &PM_Data.pm10_cf1},
	{.General_item_text = "PM1.0_Env:%d",.General_callback = NULL,.General_SubMenuPage = NULL,.List_BoolRadioBox = NULL,.General_Value = &PM_Data.pm1_0_env},
	{.General_item_text = "PM2.5_Env:%d",.General_callback = NULL,.General_SubMenuPage = NULL,.List_BoolRadioBox = NULL,.General_Value = &PM_Data.pm2_5_env},
	{.General_item_text = "PM10_Env:%d",.General_callback = NULL,.General_SubMenuPage = NULL,.List_BoolRadioBox = NULL,.General_Value = &PM_Data.pm10_env},
	{.General_item_text = "Particles0.3:%d",.General_callback = NULL,.General_SubMenuPage = NULL,.List_BoolRadioBox = NULL,.General_Value = &PM_Data.particles0_3},
	{.General_item_text = "Particles0.5:%d",.General_callback = NULL,.General_SubMenuPage = NULL,.List_BoolRadioBox = NULL,.General_Value = &PM_Data.particles0_5},
	{.General_item_text = "Particles1.0:%d",.General_callback = NULL,.General_SubMenuPage = NULL,.List_BoolRadioBox = NULL,.General_Value = &PM_Data.particles1_0},
	{.General_item_text = "Particles2.5:%d",.General_callback = NULL,.General_SubMenuPage = NULL,.List_BoolRadioBox = NULL,.General_Value = &PM_Data.particles2_5},
	{.General_item_text = "Particles5.0:%d",.General_callback = NULL,.General_SubMenuPage = NULL,.List_BoolRadioBox = NULL,.General_Value = &PM_Data.particles5_0},
	{.General_item_text = "Particles10:%d",.General_callback = NULL,.General_SubMenuPage = NULL,.List_BoolRadioBox = NULL,.General_Value = &PM_Data.particles10},
	{.General_item_text = "数据说明",.General_callback = NULL,.General_SubMenuPage = &DataExplanationPage,.List_BoolRadioBox = NULL},		
	{.General_item_text = "[返回]",.General_callback = OLED_UI_Back,.General_SubMenuPage = NULL,.List_BoolRadioBox = NULL},
	{.General_item_text = NULL},/*最后一项的General_item_text置为NULL，表示该项为分割线*/
};
MenuItem DataExplanationItems[] = {
	{.General_item_text = "Cf1:出厂校准值(标准环境基准 单位ug/m3 )",.General_callback = NULL,.General_SubMenuPage = NULL,.List_BoolRadioBox = NULL},
	{.General_item_text = "Env:实时测量值(动态环境校准 单位ug/m3 )",.General_callback = NULL,.General_SubMenuPage = NULL,.List_BoolRadioBox = NULL},
	{.General_item_text = "ParticlesX:0.1升空气中直径在 X um以上颗粒物个数",.General_callback = NULL,.General_SubMenuPage = NULL,.List_BoolRadioBox = NULL},
	{.General_item_text = "[返回]",.General_callback = OLED_UI_Back,.General_SubMenuPage = NULL,.List_BoolRadioBox = NULL},
	{.General_item_text = NULL},/*最后一项的General_item_text置为NULL，表示该项为分割线*/
};

MenuItem ErrorTypeExplanationItems[] = {
	{.General_item_text = "0：无错误",.General_callback = NULL,.General_SubMenuPage = NULL,.List_BoolRadioBox = NULL},	
	{.General_item_text = "1：数据无法发送！",.General_callback = NULL,.General_SubMenuPage = NULL,.List_BoolRadioBox = NULL},
	{.General_item_text = "2：数据无法接收！",.General_callback = NULL,.General_SubMenuPage = NULL,.List_BoolRadioBox = NULL},
	{.General_item_text = "3：扬尘数据异常！",.General_callback = NULL,.General_SubMenuPage = NULL,.List_BoolRadioBox = NULL},
	{.General_item_text = "4：噪音数据异常！",.General_callback = NULL,.General_SubMenuPage = NULL,.List_BoolRadioBox = NULL},
	{.General_item_text = "5：温湿度数据异常！",.General_callback = NULL,.General_SubMenuPage = NULL,.List_BoolRadioBox = NULL},
	{.General_item_text = "[返回]",.General_callback = OLED_UI_Back,.General_SubMenuPage = NULL,.List_BoolRadioBox = NULL},
	{.General_item_text = NULL},/*最后一项的General_item_text置为NULL，表示该项为分割线*/
};
MenuItem WarningTypeExplanationItems[] = {
	{.General_item_text = "0：无警报",.General_callback = NULL,.General_SubMenuPage = NULL,.List_BoolRadioBox = NULL},	
	{.General_item_text = "1：扬尘过大！",.General_callback = NULL,.General_SubMenuPage = NULL,.List_BoolRadioBox = NULL},
	{.General_item_text = "2：噪音过高！",.General_callback = NULL,.General_SubMenuPage = NULL,.List_BoolRadioBox = NULL},
	{.General_item_text = "3：扬尘过大且噪音过高！",.General_callback = NULL,.General_SubMenuPage = NULL,.List_BoolRadioBox = NULL},
	{.General_item_text = "[返回]",.General_callback = OLED_UI_Back,.General_SubMenuPage = NULL,.List_BoolRadioBox = NULL},
	{.General_item_text = NULL},/*最后一项的General_item_text置为NULL，表示该项为分割线*/
};

//MenuItem SmallAreaMenuItems[] = {
//	{.General_item_text = "中文文本",.General_callback = NULL,.General_SubMenuPage = NULL,.List_BoolRadioBox = NULL},
//	{.General_item_text = "English Text",.General_callback = NULL,.General_SubMenuPage = NULL,.List_BoolRadioBox = NULL},
//	{.General_item_text = "1234567890",.General_callback = NULL,.General_SubMenuPage = NULL,.List_BoolRadioBox = NULL},
//	{.General_item_text = "ABCDEFGHIJKLMNOPQRSTUVWXYZ",.General_callback = NULL,.General_SubMenuPage = NULL,.List_BoolRadioBox = NULL},
//	{.General_item_text = "abcdefghijklmnopqrstuvwxyz",.General_callback = NULL,.General_SubMenuPage = NULL,.List_BoolRadioBox = NULL},
//	{.General_item_text = ",.[]!@#$+-/^&*()",.General_callback = NULL,.General_SubMenuPage = NULL,.List_BoolRadioBox = NULL},
//	{.General_item_text = "中文文本",.General_callback = NULL,.General_SubMenuPage = NULL,.List_BoolRadioBox = NULL},
//	{.General_item_text = "English Text",.General_callback = NULL,.General_SubMenuPage = NULL,.List_BoolRadioBox = NULL},
//	{.General_item_text = "1234567890",.General_callback = NULL,.General_SubMenuPage = NULL,.List_BoolRadioBox = NULL},
//	{.General_item_text = "ABCDEFGHIJKLMNOPQRSTUVWXYZ",.General_callback = NULL,.General_SubMenuPage = NULL,.List_BoolRadioBox = NULL},
//	{.General_item_text = "abcdefghijklmnopqrstuvwxyz",.General_callback = NULL,.General_SubMenuPage = NULL,.List_BoolRadioBox = NULL},
//	{.General_item_text = ",.[]!@#$+-/^&*()",.General_callback = NULL,.General_SubMenuPage = NULL,.List_BoolRadioBox = NULL},
//	{.General_item_text = "[返回]",.General_callback = OLED_UI_Back,.General_SubMenuPage = NULL,.List_BoolRadioBox = NULL},
//	{.General_item_text = NULL},/*最后一项的General_item_text置为NULL，表示该项为分割线*/
//};



MenuPage MainMenuPage = {
	//通用属性，必填
	.General_MenuType = MENU_TYPE_TILES,  		 //菜单类型为磁贴类型
	.General_CursorStyle = NOT_SHOW,			 //光标类型
	.General_FontSize = OLED_UI_FONT_16,			//字高
	.General_ParentMenuPage = NULL,				//由于这是根菜单，所以父菜单为NULL,即由什么而来
	.General_LineSpace = 5,						//磁贴间距 单位：像素（对于磁贴类型菜单，此值表示每个磁贴之间的间距，对于列表类型菜单，此值表示行间距）
	.General_MoveStyle = UNLINEAR,				//移动方式，简单理解就是切换后图标抖动与不抖动的区别
	.General_MovingSpeed = SPEED,					//动画移动速度(此值根据实际效果调整)
	.General_ShowAuxiliaryFunction = MainAuxFunc,		 //显示辅助函数  
	.General_MenuItems = MainMenuItems,			//菜单项内容数组   

	//特殊属性，根据.General_MenuType的类型选择
	.Tiles_ScreenHeight = 64,					//屏幕高度
	.Tiles_ScreenWidth = 128,						//屏幕宽度
	.Tiles_TileWidth = 32,						 //磁贴宽度
	.Tiles_TileHeight = 32,						 //磁贴高度
};


MenuPage SettingsMenuPage = {
	//通用属性，必填
	.General_MenuType = MENU_TYPE_LIST,  		 //菜单类型为列表类型
	.General_CursorStyle = UNDERLINE,	 //光标类型为线型
	.General_FontSize = OLED_UI_FONT_12,			//字高
	.General_ParentMenuPage = &MainMenuPage,		 //父菜单为主菜单
	.General_LineSpace = 4,						//行间距 单位：像素
	.General_MoveStyle = UNLINEAR,				//移动方式为非线性曲线动画
	.General_MovingSpeed = SPEED,					//动画移动速度(此值根据实际效果调整)
	.General_ShowAuxiliaryFunction = SettingAuxFunc,		 //显示辅助函数
	.General_MenuItems = SettingsMenuItems,		 //菜单项内容数组

	//特殊属性，根据.General_MenuType的类型选择
	.List_MenuArea = {32, 0, 95, 64},			 //列表显示区域
	.List_IfDrawFrame = false,					 //是否显示边框
	.List_IfDrawLinePerfix = true,				 //是否显示行前缀
	.List_StartPointX = 4,                        //列表起始点X坐标
	.List_StartPointY = 2,                        //列表起始点Y坐标
};

MenuPage Monitor_Station_MenuPage = {
	//通用属性，必填
	.General_MenuType = MENU_TYPE_LIST,  		 //菜单类型为列表类型
	.General_CursorStyle = REVERSE_ROUNDRECTANGLE,	 //光标类型为线型
	.General_FontSize = OLED_UI_FONT_12,			//字高
	.General_ParentMenuPage = &MainMenuPage,		 //父菜单为主菜单
	.General_LineSpace = 4,						//行间距 单位：像素
	.General_MoveStyle = UNLINEAR,				//移动方式为非线性曲线动画
	.General_MovingSpeed = SPEED,					//动画移动速度(此值根据实际效果调整)
	.General_ShowAuxiliaryFunction = NULL,		 //显示辅助函数
	.General_MenuItems = Monitor_Station_MenuItems,		 //菜单项内容数组

	//特殊属性，根据.General_MenuType的类型选择
	.List_MenuArea = {1, 1, 128, 64},			 //列表显示区域
	.List_IfDrawFrame = false,					 //是否显示边框
	.List_IfDrawLinePerfix = true,				 //是否显示行前缀
	.List_StartPointX = 4,                        //列表起始点X坐标
	.List_StartPointY = 2,                        //列表起始点Y坐标
};

MenuPage AboutThisSystemPage = {
	//通用属性，必填
	.General_MenuType = MENU_TYPE_LIST,  		 //菜单类型为列表类型
	.General_CursorStyle = REVERSE_ROUNDRECTANGLE,	 //光标类型为圆角矩形
	.General_FontSize = OLED_UI_FONT_12,			//字高
	.General_ParentMenuPage = &MoreMenuPage,		 //父菜单为主菜单
	.General_LineSpace = 4,						//行间距 单位：像素
	.General_MoveStyle = UNLINEAR,				//移动方式为非线性曲线动画
	.General_MovingSpeed = SPEED,					//动画移动速度(此值根据实际效果调整)
	.General_ShowAuxiliaryFunction = AboutOLED_UIAuxFunc,		 //显示辅助函数
	.General_MenuItems = AboutThisSystemuItems,		 //菜单项内容数组

	//特殊属性，根据.General_MenuType的类型选择
	.List_MenuArea = {0, 0, 105, 64},			 //列表显示区域
	.List_IfDrawFrame = false,					 //是否显示边框
	.List_IfDrawLinePerfix = false,				 //是否显示行前缀
	.List_StartPointX = 4,                        //列表起始点X坐标
	.List_StartPointY = 2,                        //列表起始点Y坐标

};
MenuPage MoreMenuPage = {
	//通用属性，必填
	.General_MenuType = MENU_TYPE_LIST,  		 //菜单类型为列表类型
	.General_CursorStyle = REVERSE_ROUNDRECTANGLE,	 //光标类型为圆角矩形
	.General_FontSize = OLED_UI_FONT_12,			//字高
	.General_ParentMenuPage = &MainMenuPage,		 //父菜单为主菜单
	.General_LineSpace = 4,						//行间距 单位：像素
	.General_MoveStyle = UNLINEAR,				//移动方式为非线性曲线动画
	.General_MovingSpeed = SPEED,					//动画移动速度(此值根据实际效果调整)
	.General_ShowAuxiliaryFunction = NULL,		 //显示辅助函数
	.General_MenuItems = MoreMenuItems,		 //菜单项内容数组

	//特殊属性，根据.General_MenuType的类型选择
	.List_MenuArea = {1, 1, 128, 64},			 //列表显示区域
	.List_IfDrawFrame = false,					 //是否显示边框
	.List_IfDrawLinePerfix = true,				 //是否显示行前缀
	.List_StartPointX = 4,                        //列表起始点X坐标
	.List_StartPointY = 2,                        //列表起始点Y坐标

};
MenuPage MoreDustDataPage = {
	//通用属性，必填
	.General_MenuType = MENU_TYPE_LIST,  		 //菜单类型为列表类型
	.General_CursorStyle = REVERSE_ROUNDRECTANGLE,	 //光标类型为圆角矩形
	.General_FontSize = OLED_UI_FONT_12,			//字高
	.General_ParentMenuPage = &Monitor_Station_MenuPage,		 //父菜单为监控台菜单
	.General_LineSpace = 4,						//行间距 单位：像素
	.General_MoveStyle = UNLINEAR,				//移动方式为非线性曲线动画
	.General_MovingSpeed = SPEED,					//动画移动速度(此值根据实际效果调整)
	.General_ShowAuxiliaryFunction = NULL,		 //显示辅助函数
	.General_MenuItems = MoreDustDataItems,		 //菜单项内容数组

	//特殊属性，根据.General_MenuType的类型选择
	.List_MenuArea = {0, 0, 128, 64},			 //列表显示区域
	.List_IfDrawFrame = false,					 //是否显示边框
	.List_IfDrawLinePerfix = true,				 //是否显示行前缀
	.List_StartPointX = 4,                        //列表起始点X坐标
	.List_StartPointY = 2,                        //列表起始点Y坐标

};
MenuPage DataExplanationPage = {
	//通用属性，必填
	.General_MenuType = MENU_TYPE_LIST,  		 //菜单类型为列表类型
	.General_CursorStyle = REVERSE_ROUNDRECTANGLE,	 //光标类型为圆角矩形
	.General_FontSize = OLED_UI_FONT_12,			//字高
	.General_ParentMenuPage = &MoreDustDataPage,		 //父菜单为更多扬尘数据菜单
	.General_LineSpace = 4,						//行间距 单位：像素
	.General_MoveStyle = UNLINEAR,				//移动方式为非线性曲线动画
	.General_MovingSpeed = SPEED,					//动画移动速度(此值根据实际效果调整)
	.General_ShowAuxiliaryFunction = NULL,		 //显示辅助函数
	.General_MenuItems = DataExplanationItems,		 //菜单项内容数组

	//特殊属性，根据.General_MenuType的类型选择
	.List_MenuArea = {0, 0, 128, 64},			 //列表显示区域
	.List_IfDrawFrame = false,					 //是否显示边框
	.List_IfDrawLinePerfix = true,				 //是否显示行前缀
	.List_StartPointX = 4,                        //列表起始点X坐标
	.List_StartPointY = 2,                        //列表起始点Y坐标

};





MenuPage ErrorMenuPage = {
	//通用属性，必填
	.General_MenuType = MENU_TYPE_LIST,  		 //菜单类型为列表类型
	.General_CursorStyle = REVERSE_ROUNDRECTANGLE,	 //光标类型为圆角矩形
	.General_FontSize = OLED_UI_FONT_12,			//字高
	.General_ParentMenuPage = &MoreMenuPage,		 //父菜单为主菜单
	.General_LineSpace = 4,						//行间距 单位：像素
	.General_MoveStyle = UNLINEAR,				//移动方式为非线性曲线动画
	.General_MovingSpeed = SPEED,					//动画移动速度(此值根据实际效果调整)
	.General_ShowAuxiliaryFunction = NULL,		 //显示辅助函数
	.General_MenuItems = ErrorMenuItems,		 //菜单项内容数组

	//特殊属性，根据.General_MenuType的类型选择
	.List_MenuArea = {0, 0, 128, 64},			 //列表显示区域
	.List_IfDrawFrame = false,					 //是否显示边框
	.List_IfDrawLinePerfix = true,				 //是否显示行前缀
	.List_StartPointX = 4,                        //列表起始点X坐标
	.List_StartPointY = 2,                        //列表起始点Y坐标

};

MenuPage WarningMenuPage = {
	//通用属性，必填
	.General_MenuType = MENU_TYPE_LIST,  		 //菜单类型为列表类型
	.General_CursorStyle = REVERSE_ROUNDRECTANGLE,	 //光标类型为圆角矩形
	.General_FontSize = OLED_UI_FONT_12,			//字高
	.General_ParentMenuPage = &MoreMenuPage,		 //父菜单为主菜单
	.General_LineSpace = 4,						//行间距 单位：像素
	.General_MoveStyle = UNLINEAR,				//移动方式为非线性曲线动画
	.General_MovingSpeed = SPEED,					//动画移动速度(此值根据实际效果调整)
	.General_ShowAuxiliaryFunction = NULL,		 //显示辅助函数
	.General_MenuItems = WarningMenuItems,		 //菜单项内容数组

	//特殊属性，根据.General_MenuType的类型选择
	.List_MenuArea = {0, 0, 128, 64},			 //列表显示区域
	.List_IfDrawFrame = false,					 //是否显示边框
	.List_IfDrawLinePerfix = true,				 //是否显示行前缀
	.List_StartPointX = 4,                        //列表起始点X坐标
	.List_StartPointY = 2,                        //列表起始点Y坐标

};

MenuPage ErrorTypeExplanationPage = {
	//通用属性，必填
	.General_MenuType = MENU_TYPE_LIST,  		 //菜单类型为列表类型
	.General_CursorStyle = REVERSE_ROUNDRECTANGLE,	 //光标类型为圆角矩形
	.General_FontSize = OLED_UI_FONT_12,			//字高
	.General_ParentMenuPage = &ErrorMenuPage,		 //父菜单为更多错误日志菜单
	.General_LineSpace = 4,						//行间距 单位：像素
	.General_MoveStyle = UNLINEAR,				//移动方式为非线性曲线动画
	.General_MovingSpeed = SPEED,					//动画移动速度(此值根据实际效果调整)
	.General_ShowAuxiliaryFunction = NULL,		 //显示辅助函数
	.General_MenuItems = ErrorTypeExplanationItems,		 //菜单项内容数组

	//特殊属性，根据.General_MenuType的类型选择
	.List_MenuArea = {0, 0, 128, 64},			 //列表显示区域
	.List_IfDrawFrame = false,					 //是否显示边框
	.List_IfDrawLinePerfix = true,				 //是否显示行前缀
	.List_StartPointX = 4,                        //列表起始点X坐标
	.List_StartPointY = 2,                        //列表起始点Y坐标

};
MenuPage WarningTypeExplanationPage = {
	//通用属性，必填
	.General_MenuType = MENU_TYPE_LIST,  		 //菜单类型为列表类型
	.General_CursorStyle = REVERSE_ROUNDRECTANGLE,	 //光标类型为圆角矩形
	.General_FontSize = OLED_UI_FONT_12,			//字高
	.General_ParentMenuPage = &WarningMenuPage,		 //父菜单为更多错误日志菜单
	.General_LineSpace = 4,						//行间距 单位：像素
	.General_MoveStyle = UNLINEAR,				//移动方式为非线性曲线动画
	.General_MovingSpeed = SPEED,					//动画移动速度(此值根据实际效果调整)
	.General_ShowAuxiliaryFunction = NULL,		 //显示辅助函数
	.General_MenuItems = WarningTypeExplanationItems,		 //菜单项内容数组

	//特殊属性，根据.General_MenuType的类型选择
	.List_MenuArea = {0, 0, 128, 64},			 //列表显示区域
	.List_IfDrawFrame = false,					 //是否显示边框
	.List_IfDrawLinePerfix = true,				 //是否显示行前缀
	.List_StartPointX = 4,                        //列表起始点X坐标
	.List_StartPointY = 2,                        //列表起始点Y坐标

};
//MenuPage SmallAreaMenuPage = {
//	//通用属性，必填
//	.General_MenuType = MENU_TYPE_LIST,  		 //菜单类型为列表类型
//	.General_CursorStyle = REVERSE_ROUNDRECTANGLE,	 //光标类型为圆角矩形
//	.General_FontSize = OLED_UI_FONT_12,			//字高
//	.General_ParentMenuPage = &MoreMenuPage,		 //父菜单为主菜单
//	.General_LineSpace = 6,						//行间距 单位：像素
//	.General_MoveStyle = UNLINEAR,				//移动方式为非线性曲线动画
//	.General_MovingSpeed = SPEED,					//动画移动速度(此值根据实际效果调整)
//	.General_ShowAuxiliaryFunction = NULL,		 //显示辅助函数
//	.General_MenuItems = SmallAreaMenuItems,		 //菜单项内容数组

//	//特殊属性，根据.General_MenuType的类型选择
//	.List_MenuArea = {34, 20, 60, 36},			 //列表显示区域
//	.List_IfDrawFrame = true,					 //是否显示边框
//	.List_IfDrawLinePerfix = true,				 //是否显示行前缀
//	.List_StartPointX = 4,                        //列表起始点X坐标
//	.List_StartPointY = 2,                        //列表起始点Y坐标

//};

