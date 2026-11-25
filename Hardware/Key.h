#ifndef __KEY_H
#define __KEY_H
#include <stdint.h>
extern volatile uint8_t Key_Num;
uint8_t Key_GetNum(void);
void Key_Tick(void);
void Key3_Tick(void);
uint8_t Key_GetNum_RTOS(void);
#endif
