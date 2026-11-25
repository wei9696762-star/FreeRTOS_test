# freertos测试用到了任务管理，队列和计数信号量来改造裸机手表项目
**大概思路**
<img width="2417" height="1779" alt="图片1" src="https://github.com/user-attachments/assets/fa703d2c-fa1e-49fb-be07-bfb4284a3d48" />
**主要函数**
**freertos_demo.c**
```c
#include "freertos_demo.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "OLED.h"
#include "MPU6050.h"
#include "gpio.h"
#include "menu.h"     
#include "key.h"      
#include "adc.h"      
#include "tim.h"      
#include "semphr.h"

// 消息队列句柄
QueueHandle_t Key_Queue;

//计数信号量句柄
SemaphoreHandle_t Key_Sem;

// 任务句柄
TaskHandle_t Start_Task_Handle;
TaskHandle_t Key_Task_Handle;
TaskHandle_t ShowTime_Task_Handle;
TaskHandle_t Menu_Task_Handle;
TaskHandle_t StopWatch_Task_Handle;
TaskHandle_t LED_Task_Handle;
TaskHandle_t MPU6050_Task_Handle;
TaskHandle_t Game_Task_Handle;
TaskHandle_t Emoji_Task_Handle;
TaskHandle_t Gradienter_Task_Handle;
TaskHandle_t SetTime_Task_Handle;

#define KEY_TASK_PRIORITY       4
#define KEY_STK_SIZE            128  

#define StopWatch_TASK_PRIORITY   3
#define LED_TASK_PRIORITY         3
#define ShowTime_TASK_PRIORITY    3
#define Menu_TASK_PRIORITY        3
#define Game_TASK_PRIORITY        3
#define Emoji_TASK_PRIORITY       3
#define MPU6050_TASK_PRIORITY     3
#define Gradienter_TASK_PRIORITY  3
#define SetTime_TASK_PRIORITY     3

#define StopWatch_STK_SIZE      128  
#define LED_STK_SIZE            128
#define ShowTime_STK_SIZE       256
#define Menu_STK_SIZE           256  
#define Game_STK_SIZE           256  
#define Emoji_STK_SIZE          256  
#define MPU6050_STK_SIZE        350  
#define Gradienter_STK_SIZE     350  
#define SetTime_STK_SIZE        350  

#define START_TASK_PRIORITY     1
#define START_STK_SIZE          128  

//任务函数
void Start_Task(void *pvParameters);
void Key_Task(void *pvParameters);
void ShowTime_Task(void *pvParameters);
void Menu_Task(void *pvParameters);
void StopWatch_Task(void *pvParameters);
void LED_Task(void *pvParameters);
void MPU6050_Task(void *pvParameters);
void Game_Task(void *pvParameters);
void Emoji_Task(void *pvParameters);
void Gradienter_Task(void *pvParameters);
void SetTime_Task(void *pvParameters);

//FreeRTOS 启动函数
void FreeRTOS_Start(void)
{
    /* 应用初始化*/
    OLED_Init();
    OLED_Clear();
    MPU6050_Init();
    HAL_ADCEx_Calibration_Start(&hadc1);    /* 使用前校准 ADC */

    // 创建按键队列
    Key_Queue = xQueueCreate(5, sizeof(uint8_t));
	  // 创建信号量
		Key_Sem   = xSemaphoreCreateCounting(8, 0);  
	

    // 创建开始任务
    xTaskCreate(Start_Task,"Start_Task",START_STK_SIZE,NULL,START_TASK_PRIORITY,&Start_Task_Handle);

    vTaskStartScheduler();
}

//启动任务
void Start_Task(void *pvParameters)
{
    taskENTER_CRITICAL(); 

    // 按键任务
    xTaskCreate(Key_Task, "Key_Task", KEY_STK_SIZE, NULL, KEY_TASK_PRIORITY, &Key_Task_Handle);

    xTaskCreate(ShowTime_Task, "Time_Task", ShowTime_STK_SIZE, NULL, ShowTime_TASK_PRIORITY, &ShowTime_Task_Handle);
    xTaskCreate(Menu_Task,     "Menu_Task", Menu_STK_SIZE, NULL, Menu_TASK_PRIORITY, &Menu_Task_Handle);
    xTaskCreate(Game_Task,     "Game",      Game_STK_SIZE, NULL, Game_TASK_PRIORITY, &Game_Task_Handle);
    xTaskCreate(Emoji_Task,    "Emoji",     Emoji_STK_SIZE, NULL, Emoji_TASK_PRIORITY, &Emoji_Task_Handle);
    xTaskCreate(StopWatch_Task, "StopWatch", StopWatch_STK_SIZE, NULL, StopWatch_TASK_PRIORITY, &StopWatch_Task_Handle);
    xTaskCreate(LED_Task,       "LED",       LED_STK_SIZE, NULL, LED_TASK_PRIORITY, &LED_Task_Handle);
    xTaskCreate(MPU6050_Task,   "MPU6050",   MPU6050_STK_SIZE, NULL, MPU6050_TASK_PRIORITY, &MPU6050_Task_Handle);
    xTaskCreate(Gradienter_Task,"Gradienter",Gradienter_STK_SIZE, NULL, Gradienter_TASK_PRIORITY, &Gradienter_Task_Handle);
    xTaskCreate(SetTime_Task,   "SetTime",   SetTime_STK_SIZE, NULL, SetTime_TASK_PRIORITY, &SetTime_Task_Handle);

    vTaskSuspend(Menu_Task_Handle);
    vTaskSuspend(StopWatch_Task_Handle);
    vTaskSuspend(LED_Task_Handle);
    vTaskSuspend(MPU6050_Task_Handle);
    vTaskSuspend(Game_Task_Handle);
    vTaskSuspend(Emoji_Task_Handle);
    vTaskSuspend(Gradienter_Task_Handle);
    vTaskSuspend(SetTime_Task_Handle);

    vTaskDelete(Start_Task_Handle); 
    taskEXIT_CRITICAL();            
}

// 按键任务
void Key_Task(void *pvParameters)
{
    uint8_t key;
    while(1)
    {
        // 没有按键就一直阻塞在这里 
        if(xSemaphoreTake(Key_Sem, portMAX_DELAY) == pdTRUE)
        {
            key = Key_Num;
            Key_Num = 0;
            if(key != 0)
            {
                xQueueSend(Key_Queue, &key, 0);
            }
        }
    }
}

//从队列中读取键值
uint8_t Key_GetNum_RTOS(void)
{
    uint8_t key_val = 0;
    BaseType_t xStatus;
    
    // 参数 0 表示不等待，立刻返回
    xStatus = xQueueReceive(Key_Queue, &key_val, 0); 
    
    if(xStatus == pdPASS)
    {
        return key_val;
    }
    else
    {
        return 0; // 没按键时返回0
    }
}

//UI任务
void ShowTime_Task(void *pvParameters)
{
    int flag;
    while(1)
    {
        flag = First_Page_Clock(); 
        
        if(flag == 1) // 进入菜单
        {
            vTaskResume(Menu_Task_Handle);
            vTaskSuspend(NULL);
        }
        else if(flag == 2) // 进入设置
        {
             vTaskResume(SetTime_Task_Handle);
             vTaskSuspend(NULL);
        }
        vTaskDelay(5); //释放CPU
    }
}

void Menu_Task(void *pvParameters)
{
    int selection;
    while(1)
    {
        selection = Menu(); // Menu现在返回索引
        
        switch(selection)
        {
            case 0: // 返回首页
                vTaskResume(ShowTime_Task_Handle);
                vTaskSuspend(NULL);
                break;
            case 2: // 秒表
                vTaskResume(StopWatch_Task_Handle);
                vTaskSuspend(NULL);
                break;
            case 3: // 手电筒
                vTaskResume(LED_Task_Handle);
                vTaskSuspend(NULL);
                break;
            case 4: // MPU6050
                vTaskResume(MPU6050_Task_Handle);
                vTaskSuspend(NULL);
                break;
            case 5: // 游戏
                vTaskResume(Game_Task_Handle);
                vTaskSuspend(NULL);
                break;
            case 6: // 表情
                vTaskResume(Emoji_Task_Handle);
                vTaskSuspend(NULL);
                break;
            case 7: // 水平仪
                vTaskResume(Gradienter_Task_Handle);
                vTaskSuspend(NULL);
                break;
            default: 
                vTaskResume(ShowTime_Task_Handle);
                vTaskSuspend(NULL);
                break;
        }
        vTaskDelay(5);
    }
}

void StopWatch_Task(void *pvParameters)
{
    while(1)
    {
        MenuToFunction();
		StopWatch(); //除非退出此任务，否则一直会阻塞在这，所以MenuToFunction();不会被反复执行
        vTaskResume(Menu_Task_Handle);//一旦退出此任务，在裸机中需要重新初始化菜单各参数
        vTaskSuspend(NULL);//在FreeRTOS中直接回到菜单任务就好了
        vTaskDelay(5);
    }
}

void LED_Task(void *pvParameters)
{
    while(1)
    {
        MenuToFunction();
		LED();
        vTaskResume(Menu_Task_Handle);
        vTaskSuspend(NULL);
        vTaskDelay(5);
    }
}

void MPU6050_Task(void *pvParameters)
{
    while(1)
    {
		MenuToFunction();
        MPU6050();
        vTaskResume(Menu_Task_Handle);
        vTaskSuspend(NULL);
        vTaskDelay(5);
    }
}

void Game_Task(void *pvParameters)
{
    while(1)
    {
        MenuToFunction();
		Game(); 
        vTaskResume(Menu_Task_Handle);
        vTaskSuspend(NULL);
        vTaskDelay(5);
    }
}

void Emoji_Task(void *pvParameters)
{
    while(1)
    {
        MenuToFunction();
		Emoji();
        vTaskResume(Menu_Task_Handle);
        vTaskSuspend(NULL);
        vTaskDelay(5);
    }
}

void Gradienter_Task(void *pvParameters)
{
    while(1)
    {
        MenuToFunction();
		Gradienter();
        vTaskResume(Menu_Task_Handle);
        vTaskSuspend(NULL);
        vTaskDelay(5);
    }
}

void SetTime_Task(void *pvParameters)
{
    while(1)
    {
        SettingPage(); 
        vTaskResume(ShowTime_Task_Handle); 
        vTaskSuspend(NULL);
        vTaskDelay(5);
    }
}

/**
  * @brief  栈溢出回调（configCHECK_FOR_STACK_OVERFLOW = 2 时必须实现，否则链接报 L6218E）
  * @note   调试时在本函数第一行打断点，看 pcTaskName 就知道哪个任务的栈爆了。
  */
void vApplicationStackOverflowHook( TaskHandle_t xTask, char *pcTaskName )
{
  (void)xTask;
  (void)pcTaskName;
  taskDISABLE_INTERRUPTS();
  for (;;)
  {
  }
}

```


**key.c**
```c
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
```
