/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "queue.h"
#include "event_groups.h"
#include "semphr.h"

#include "driver_led.h"
#include "driver_key.h"
#include "driver_dht11.h"
#include "driver_light_sensor.h"
#include "driver_oled.h"
#include "driver_motor.h"
#include "esp8266.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

// 定义一个枚举类型用于区分传感器数据的类型
typedef enum
{
  data_type_dht11,
  data_type_light
}data_type;   

// 定义一个联合体用于存储不同类型的传感器数据
typedef struct
{
  data_type type;
  union{
    dht11_result dht;
    light_data light;
  }all_data;
}sensor_data;   

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define Queue_Length    5               // 传感器数据队列长度
#define Queue_Timeout   100             // 发送数据到队列的超时时间（ms） 

#define Event_Light_Turn  (1UL << 0)    // 事件组位：LED状态切换事件
#define Event_Light_On    (1UL << 1)    // 事件组位：LED开事件
#define Event_Light_Off   (1UL << 2)    // 事件组位：LED关事件

TaskHandle_t ledTaskHandle;
TaskHandle_t SensorTaskHandle;
TaskHandle_t DisplayTaskHandle;
TaskHandle_t KeyTaskHandle;
TaskHandle_t ControlLedTaskHandle = NULL;
TaskHandle_t ControlFanTaskHandle = NULL;
TaskHandle_t ConnectWifiTaskHandle = NULL;
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
QueueHandle_t Sensor_Queue;           // 传感器数据队列句柄
QueueHandle_t Key_Queueset;           // 按键事件队列集句柄

SemaphoreHandle_t Key1_Semaphore;     // 按键1信号量
SemaphoreHandle_t Key2_Semaphore;     // 按键2信号量
SemaphoreHandle_t Esp_RxSem;          // ESP8266接收信号量
SemaphoreHandle_t Post_Mutex;         // 上传数据互斥量

EventGroupHandle_t Light_EventGroup;  // LED事件组句柄，用于控制LED状态切换、开、关事件
/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
void LED_Task(void *pvParameters);
void Sensor_Task(void *pvParameters);
void Display_Task(void *pvParameters);
void Key_Task(void *pvParameters);
void Control_Led_Task(void *pvParameters);
void Control_Fan_Task(void *pvParameters);
/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  Key1_Semaphore = xSemaphoreCreateBinary();
  Key2_Semaphore = xSemaphoreCreateBinary();
  Esp_RxSem = xSemaphoreCreateBinary();
  Post_Mutex = xSemaphoreCreateMutex();
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  Sensor_Queue = xQueueCreate(Queue_Length,sizeof(sensor_data));
  Key_Queueset = xQueueCreateSet(2);
  xQueueAddToSet(Key1_Semaphore,Key_Queueset);
  xQueueAddToSet(Key2_Semaphore,Key_Queueset);
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  Light_EventGroup = xEventGroupCreate();
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN StartDefaultTask */
	BaseType_t xResult;
	xTaskCreate((TaskFunction_t)LED_Task,
            (const char* )"LED_Task", 
            (uint16_t )128, 
            (void* )NULL, 
            (UBaseType_t )1,
            (TaskHandle_t* )&ledTaskHandle);
  xTaskCreate((TaskFunction_t)Sensor_Task,
            (const char* )"Sensor_Task", 
            (uint16_t )128, 
            (void* )NULL, 
            (UBaseType_t )4,
            (TaskHandle_t* )&SensorTaskHandle);
	xTaskCreate((TaskFunction_t)Display_Task,
            (const char* )"Display_Task", 
            (uint16_t )128, 
            (void* )NULL, 
            (UBaseType_t )5,
            (TaskHandle_t* )&DisplayTaskHandle);
  xTaskCreate((TaskFunction_t)Key_Task,
            (const char* )"Key_Task", 
            (uint16_t )128, 
            (void* )NULL, 
            (UBaseType_t )5,
            (TaskHandle_t* )&KeyTaskHandle);
  xTaskCreate((TaskFunction_t)Control_Led_Task,
            (const char* )"Control_Led_Task", 
            (uint16_t )128, 
            (void* )NULL, 
            (UBaseType_t )5,
            (TaskHandle_t* )&ControlLedTaskHandle);
  xTaskCreate((TaskFunction_t)Control_Fan_Task,
            (const char* )"Control_Fan_Task", 
            (uint16_t )128, 
            (void* )NULL, 
            (UBaseType_t )5,
            (TaskHandle_t* )&ControlFanTaskHandle);
  xTaskCreate((TaskFunction_t)Connect_Wifi_Task,
            (const char* )"Connect_Wifi_Task", 
            (uint16_t )256, 
            (void* )NULL, 
            (UBaseType_t )5,
            (TaskHandle_t* )&ConnectWifiTaskHandle);

	vTaskDelete(NULL); 
  /* Infinite loop */
  for(;;)
  {
  }
  /* USER CODE END StartDefaultTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/**
  * @brief  GPIO中断回调函数，由HAL库在GPIO中断发生时调用，根据中断引脚不同释放不同的信号量
  * @param  GPIO_Pin: GPIO引脚
  * @retval 无
  */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	switch(GPIO_Pin)
	{
		case Key1_Pin :
			xSemaphoreGiveFromISR(Key1_Semaphore,&xHigherPriorityTaskWoken);
			break;
		
		case Key2_Pin :
			xSemaphoreGiveFromISR(Key2_Semaphore,&xHigherPriorityTaskWoken);
			break;
	}
  portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

/**
  * @brief  LED任务函数，根据从事件组中获取的事件控制LED的状态
  * @param  pvParameters: 参数
  * @retval None
  */
void LED_Task(void *pvParameters)
{
    Led_Init();

    EventBits_t xbit;
    const EventBits_t AllBits = Event_Light_Turn | Event_Light_On | Event_Light_Off;

    while(1)
    {       
        xbit = xEventGroupWaitBits(Light_EventGroup,AllBits,pdTRUE,pdFALSE,portMAX_DELAY);

        if((xbit & Event_Light_Turn) != 0)
        {
          Led_Turn();
        }

        if((xbit & Event_Light_On) != 0)
        {
          Led_On();
        }

        if((xbit & Event_Light_Off) != 0)
        {
          Led_Off();
        }
    }
}

/**
  * @brief  传感器任务函数，定时读取DHT11和光照传感器的数据，将数据发送到队列，并通过任务通知将数据发送给控制风扇和LED的任务
  * @param  pvParameters: 参数
  * @retval None
  */
void Sensor_Task(void *pvParameters)
{
  DHT11_Init();
  LightSensor_Init();
  
  sensor_data buf;
  dht11_result result;
  light_data pdata;
  uint32_t notify_data;
  int light_notify_data;

  while(1)
  {
    if(DHT11_Read_Result(&result) == DHT11_TRUE)
    {
        buf.type = data_type_dht11;
        buf.all_data.dht = result;

        xQueueSend(Sensor_Queue,&buf,Queue_Timeout);
        MQTT_Post("temp",(char)buf.all_data.dht.temp);
        MQTT_Post("humi",(char)buf.all_data.dht.humi);

        if(ControlFanTaskHandle != NULL)
        {
            notify_data = (uint32_t)(result.temp * 10);
            xTaskNotify(ControlFanTaskHandle,notify_data,eSetValueWithOverwrite);
        }
    }
    if(LightSensor_Read(&pdata) == Light_OK)
    {
        buf.type = data_type_light;
        buf.all_data.light = pdata;

        xQueueSend(Sensor_Queue,&buf,Queue_Timeout);
        MQTT_Post("light",(char)buf.all_data.light.data);

        if(ControlLedTaskHandle != NULL)
        {
            light_notify_data = pdata.data;
            xTaskNotify(ControlLedTaskHandle,light_notify_data,eSetValueWithOverwrite);
        }
    }

    vTaskDelay(1000);
  }
}

/**
  * @brief  显示任务函数，从队列中接收传感器数据并在OLED上显示
  * @param  pvParameters: 参数
  * @retval None
  */
void Display_Task(void *pvParameters)
{
  OLED_Clear();
  
  int len,light_len;
  sensor_data buf;
	
  len = OLED_PrintString(0,0,"temp:");
  OLED_PrintString(0,2,"humi:");
  light_len = OLED_PrintString(0,4,"light:");
	
  while(1)
  {
    if(xQueueReceive(Sensor_Queue,&buf,portMAX_DELAY) == pdTRUE)
    {
      switch (buf.type)
      {
        case data_type_dht11:       
          OLED_PrintFloat(len,0,buf.all_data.dht.temp,1);
          OLED_PrintFloat(len,2,buf.all_data.dht.humi,1);
          break;
        
        case data_type_light:
          OLED_PrintSignedVal(light_len,4,buf.all_data.light.data);
          break;
      }
    }
  }
}

/**
  * @brief  按键任务函数，处理按键输入并触发相应事件：
  *         长按第二个按键切换风扇的自动/手动模式，短按第二个按键在手动模式下切换风扇的档位，
  *         按下第一个按键触发LED状态切换事件
  * @param  pvParameters: 参数
  * @retval None
  */
void Key_Task(void *pvParameters)
{
  Key_Init();

  QueueSetMemberHandle_t Key_Member;
  uint8_t currentState, lastStateA = 1, lastStateB = 1;

  while(1)
  {
    Key_Member = xQueueSelectFromSet(Key_Queueset,portMAX_DELAY);
    if(Key_Member == Key1_Semaphore)
    {
      vTaskDelay(20);
      currentState = HAL_GPIO_ReadPin(Key_Port,Key1_Pin);
      if((currentState == 0) &&(lastStateA == 1))
      {
        xEventGroupSetBits(Light_EventGroup,Event_Light_Turn);
      }
      lastStateA = currentState;
      xSemaphoreTake(Key1_Semaphore,0);
    }else if(Key_Member == Key2_Semaphore)
    {
      vTaskDelay(20);
      currentState = HAL_GPIO_ReadPin(Key_Port,Key2_Pin);
      if((currentState == 0) &&(lastStateB == 1))
      {
          uint32_t press_start_tick = xTaskGetTickCount();
          while(HAL_GPIO_ReadPin(Key_Port,Key2_Pin) == 0)
          {
              xTaskDelay(10);
          }
          uint32_t press_time = xTaskGetTickCount() - press_start_tick;
          if(press_time >= 2000)
          {
              Fan_SetControlMode(Fan_Auto);
          }else
          {
              Fan_SetControlMode(Fan_Manual);
              Fan_SwitchNextMode();
          }
          
      }
      lastStateB = currentState;
      xSemaphoreTake(Key2_Semaphore,0);
    }
  }
}

/**
  * @brief  控制LED任务函数，根据从任务通知中接收的数据控制LED状态
  * @param  pvParameters: 参数
  * @retval None
  */
void Control_Led_Task(void *pvParameters)
{
  const int light_low = 10;
  const int light_high = 90;
  int notify_data;
  BaseType_t Result;

  while(1)
  {
      Result = xTaskNotifyWait(0x00,0xffffffff,&notify_data,portMAX_DELAY);
      if(Result == pdTRUE)
      {
        if(notify_data < light_low)
        {
            xEventGroupSetBits(Light_EventGroup,Event_Light_Off);
        }else if(notify_data > light_high)
        {
            xEventGroupSetBits(Light_EventGroup,Event_Light_On);
        }
      }
  }

}

/**
  * @brief  控制风扇任务函数，根据从任务通知中接收的数据控制风扇状态
  * @param  pvParameters: 参数
  * @retval None
  */
void Control_Fan_Task(void *pvParameters)
{
    uint32_t notify_data;
    BaseType_t Result;
    float current_temp;
    Fan_AppInit();
    while(1)
    {
        Result = xTaskNotifyWait(0x00,0xffffffff,&notify_data,portMAX_DELAY);
        if(Result == pdTRUE)
        {
            current_temp = notify_data / 10.0f;
            Fan_AutoControl(current_temp);
        }

    }
}

void Connect_Wifi_Task(void *pvParameters)
{
    ESP8266_Init();
    while(MQTT_Init() != Work_Ok)
    {
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
    while(1)
    {
        BaseType_t Result = xTaskNotifyWait(0x00,0xffffffff,NULL,portMAX_DELAY);

        if(Result == pdTRUE)
        {
            while(MQTT_Init() != Work_Ok)
            {
                vTaskDelay(pdMS_TO_TICKS(5000));
            }

            extern volatile uint8_t connect_wifi_flag;
            connect_wifi_flag = 0;
        }
    }
}
/* USER CODE END Application */

