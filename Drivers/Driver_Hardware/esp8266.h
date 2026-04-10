#ifndef _ESP8266_H
#define _ESP8266_H

#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include <stdio.h>
#include "driver_oled.h"

#define Rx_Buf_Size         512
#define Esp_Dma_Rxbuf_Size  256
#define Esp_Timeout         3000
#define TEMP_RX_BUF_SIZE    1024

#define Wifi_Name           "klee"
#define Wifi_Word           "Abcd1007"
#define Equipment_Name      "test"
#define Product_ID          "sF5P0pz7WG"
#define MQTT_Token          "version=2018-10-31&res=products%2FsF5P0pz7WG%2Fdevices%2Ftest&et=1798732799&method=md5&sign=vQO5bumEwiV8hsNMx3apJg%3D%3D"
#define ONENET_Set          "$sys/" Product_ID "/" Equipment_Name "/thing/property/set" 
#define ONENET_Reply        "$sys/" Product_ID "/" Equipment_Name "/thing/property/post/reply"
#define ESP8266_Post        "$sys/" Product_ID "/" Equipment_Name "/thing/property/post"
#define ESP8266_Set         "$sys/" Product_ID "/" Equipment_Name "/thing/property/set_reply" 
#define Host                "mqtts.heclouds.com"

typedef struct 
{
    char buffer[Rx_Buf_Size];
    volatile uint16_t head;
    volatile uint16_t tail;
}USART_Rx_Buf;

typedef enum
{
    Work_Ok,
    Error
}ESP8266_Status;

extern UART_HandleTypeDef huart1;
extern SemaphoreHandle_t Esp_RxSem;
extern SemaphoreHandle_t Post_Mutex;

void ESP8266_Init(void);
void ESP8266_RxCheck(void);
ESP8266_Status MQTT_Get(const char *param,const char *value);
ESP8266_Status MQTT_Init(void);
ESP8266_Status MQTT_Post(const char *param,const char *value);

#endif 
