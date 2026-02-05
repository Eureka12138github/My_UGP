#ifndef TASK_SCHED_H
#define TASK_SCHED_H
#include "stm32f10x.h"                  // Device header
#include "stdbool.h"
#include "pms7003_drv.h"
#include "error_warning_log.h"
#include "bsp_alarm.h" 
#include "menu_data.h"
#include "xm7903_drv.h"
#include "onenet_mqtt.h"
#include "dht11_drv.h"
#include "onenet_handler.h"


void TaskHandler(void);
void TaskSchedule(void);
void DMATaskHandler(void);
void Handle_Alarm(void);
void Wrap_OLED_UI(void);
void Handle_Thresholds(void);

void Trigger_Success_Indicator(void);


#endif
