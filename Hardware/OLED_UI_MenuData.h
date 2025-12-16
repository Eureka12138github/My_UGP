#ifndef __OLED_UI_MENUDATA_H
#define __OLED_UI_MENUDATA_H
// 检测是否是C++编译器
#ifdef __cplusplus
extern "C" {
#endif
#include "OLED_UI.h"
#define Default_Dust_Limit 75		//
#define Default_Noise_Limit 70		//GB 12523-2011
	
//进行前置声明
extern MenuItem MainMenuItems[],Monitor_Station_MenuItems[],SettingsMenuItems[],AboutThisDeviceMenuItems[],
AboutOLED_UIMenuItems[],MoreMenuItems[],LongListMenuItems[],SmallAreaMenuItems[],ErrorTypeExplanationItems[]
,WarningMenuItems[],WarningTypeExplanationItems[];
extern MenuPage MainMenuPage,SettingsMenuPage,Monitor_Station_MenuPage,MoreDustDataPage,DataExplanationPage,
AboutThisSystemPage,MoreMenuPage,ErrorMenuPage,ErrorTypeExplanationPage,WarningTypeExplanationPage,SmallAreaMenuPage,WarningMenuPage;
extern PM_SensorData PM_Data;


#ifdef __cplusplus
}  // extern "C"
#endif

#endif
