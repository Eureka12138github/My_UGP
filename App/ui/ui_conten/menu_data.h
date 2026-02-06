#ifndef __MENU_DATA_H
#define __MENU_DATA_H
// 检测是否是C++编译器
#ifdef __cplusplus
extern "C" {
#endif
#include "oled_menu.h"

	
//进行前置声明
extern MenuItem MainMenuItems[],Monitor_Station_MenuItems[],SettingsMenuItems[],AboutThisDeviceMenuItems[],
AboutOLED_UIMenuItems[],MoreMenuItems[],LongListMenuItems[],SmallAreaMenuItems[],ErrorTypeExplanationItems[]
,WarningMenuItems[],WarningTypeExplanationItems[];
extern MenuPage MainMenuPage,SettingsMenuPage,Monitor_Station_MenuPage,MoreDustDataPage,DataExplanationPage,
AboutThisSystemPage,MoreMenuPage,ErrorMenuPage,ErrorTypeExplanationPage,WarningTypeExplanationPage,SmallAreaMenuPage,WarningMenuPage;



#ifdef __cplusplus
}  // extern "C"
#endif

#endif
