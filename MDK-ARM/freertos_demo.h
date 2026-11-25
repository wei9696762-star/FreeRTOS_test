#ifndef __FREERTOS_DEMO_H__
#define __FREERTOS_DEMO_H__

#include "FreeRTOS.h"
#include "semphr.h"

void FreeRTOS_Start(void);
extern SemaphoreHandle_t Key_Sem;
#endif
