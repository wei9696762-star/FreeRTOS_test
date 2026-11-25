#include "FreeRTOS.h"
#include "semphr.h"
#include "freertos_demo.h"
#include "gpio.h"
volatile uint8_t Key_Num;


uint8_t Key_GetNum(void)
{
	uint8_t Temp;
	if(Key_Num)
	{
		Temp=Key_Num;
		Key_Num=0;
		return Temp;
	}
	else
	{
		return 0;
	}
}

int press_time;
void Key3_Tick(void)
{
	if(HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_4) == GPIO_PIN_RESET)
	{
		press_time++;
	}
	
	if(HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_4) == GPIO_PIN_SET)
	{
		press_time=0;
	}
};

uint8_t Key_GetState(void)
{
	
	if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_1) == GPIO_PIN_RESET)
	{
		return 1;
	}
	else if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_6) == GPIO_PIN_RESET)
	{
		return 2;
	}
	else if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_4) == GPIO_PIN_RESET)
	{
		if(press_time > 1000)
		{
			return 4; // 长按
		}
		else
		{
			return 3; // 短按
		}
	}
	else
	{
		return 0;
	}
	
}



void Key_Tick(void)
{
    static uint8_t Count;
    static uint8_t CurrentState, PreState;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    Count++;
    if(Count >= 20)
    {
        Count = 0;
        PreState = CurrentState;
        CurrentState = Key_GetState();
        if(PreState != 0 && CurrentState == 0)   /* 按键松开，产生一次有效键值 */
        {
            Key_Num = PreState;

        
            xSemaphoreGiveFromISR(Key_Sem, &xHigherPriorityTaskWoken);
            portYIELD_FROM_ISR(xHigherPriorityTaskWoken);   /* 若唤醒了更高优先级任务，退出中断时切换 */
        }
    }
}
