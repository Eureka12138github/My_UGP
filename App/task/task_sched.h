#ifndef TASK_SCHED_H
#define TASK_SCHED_H
#include "stm32f10x.h"                  // Device header

void TaskHandler(void);
void TaskSchedule(void);
void DMATaskHandler(void);
void Handle_Alarm(void);
void Wrap_OLED_UI(void);
void Handle_Thresholds(void);

void Trigger_Success_Indicator(void);


#endif
