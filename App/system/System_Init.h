#ifndef  SYSTEM_INIT_H
#define SYSTEM_INIT_H
#include "stm32f10x.h"                  // Device header
#include "OLED.h"
#include "project_secrets.h"
#include "menu_data.h"
#include "onenet_mqtt.h"
#include "error_warning_log.h"
#include "bsp_alarm.h"
#include "bsp_timer.h"
#include "dht11_drv.h"
#include "bsp_xm7903.h"
void Initialize_Hardware(void);
void Initialize_System(void);



#endif
