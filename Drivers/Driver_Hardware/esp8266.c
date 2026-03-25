#include "stm32f1xx_hal.h"
#include "esp8266.h"
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include <stdio.h>

USART_Rx_Buf ESP8266_Rxbuf = {0};
uint8_t Esp_Dma_Rxbuf[Esp_Dma_Rxbuf_Size];
static uint16_t old_pos = 0;

/************构建环形缓冲区***********/
/**
  * @brief  初始化环形缓冲区
  * @param  rb: 指向要初始化的 USART_Rx_Buf 结构体的指针
  * @retval 无
  */
static Buffer_Init(USART_Rx_Buf *rb)
{
    rb->head = 0;
    rb->tail = 0;   
}

/**
  * @brief  向环形缓冲区写入一个字节数据
  * @param  rb: 指向 USART_Rx_Buf 结构体的指针
  * @param  data: 要写入的数据
  * @retval 成功返回 Work_Ok，缓冲区满则返回 Error
  */
static Buffer_Write(USART_Rx_Buf *rb,uint8_t data)
{
    uint16_t next_head = (rb->head + 1) % Rx_Buf_Size;
    if(next_head == rb->tail)
    {
        return Error;
    }
    rb->buffer[rb->head] = data;
    rb->head = next_head;
    return Work_Ok;
}

/**
  * @brief  从环形缓冲区读取一个字节数据
  * @param  rb: 指向 USART_Rx_Buf 结构体的指针
  * @param  data: 指向用于存储读出数据的变量的指针
  * @retval 成功返回 Work_Ok，缓冲区空则返回 Error
  */
static Buffer_Read(USART_Rx_Buf *rb,uint8_t *data)
{
    if(rb->head == rb->tail)
    {
        return Error;
    }
    *data = rb->buffer[rb->tail];
    rb->tail = (rb->tail + 1) % Rx_Buf_Size;
    return Work_Ok;
}

/**
  * @brief  获取环形缓冲区中当前有效数据的数量
  * @param  rb: 指向 USART_Rx_Buf 结构体的指针
  * @retval 缓冲区中有效数据的个数
  */
static Buffer_GetCount(USART_Rx_Buf *rb)
{
    if(rb->head >= rb->tail)
    {
        return rb->head - rb->tail;
    }else
    {
        return Rx_Buf_Size - rb->tail + rb->head;
    }
}

/**
  * @brief  清空环形缓冲区
  * @param  rb: 指向要清空的 USART_Rx_Buf 结构体的指针
  * @retval 无
  */
static Buffer_Clear(USART_Rx_Buf *rb)
{
    rb->head = 0;
    rb->tail = 0;
}

/************ESP8266驱动********** */
/**
  * @brief  ESP8266模块初始化
  * @note   初始化环形缓冲区，并启动UART的DMA接收及空闲中断
  * @param  无
  * @retval 无
  */
void ESP8266_Init(void)
{
    Buffer_Init(&ESP8266_Rxbuf);
    HAL_UART_Receive_DMA(&huart1,Esp_Dma_Rxbuf,Esp_Dma_Rxbuf_Size);
    __HAL_UART_ENABLE_IT(&huart1,UART_IT_IDLE);
}

/**
  * @brief  ESP8266接收数据检查与搬运函数
  * @note   通常在UART空闲中断中调用。检查DMA接收缓冲区的新数据，并将其搬运到环形缓冲区中，然后释放信号量。
  * @param  无
  * @retval 无
  */
void ESP8266_RxCheck(void)
{
    uint16_t pos = Esp_Dma_Rxbuf_Size - __HAL_DMA_GET_COUNTER(huart1.hdmarx);
    if(pos > old_pos)
    {
        for(uint16_t i = old_pos;i < pos;i++)
        {
            Buffer_Write(&ESP8266_Rxbuf,Esp_Dma_Rxbuf[i]);
        }
    }else if(pos < old_pos)
    {
        for(uint16_t i = old_pos;i < Esp_Dma_Rxbuf_Size;i++)
        {
            Buffer_Write(&ESP8266_Rxbuf,Esp_Dma_Rxbuf[i]);
        }
        for(uint16_t i = 0;i < pos;i++)
        {
            Buffer_Write(&ESP8266_Rxbuf,Esp_Dma_Rxbuf[i]);
        }
    }
    old_pos = pos;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR(Esp_RxSem,&xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

/**
  * @brief  ESP8266发送命令（无等待响应）
  * @param  cmd: 要发送的AT命令字符串
  * @retval 无
  */
static void ESP8266_SendCmd(const char *cmd)
{
    HAL_UART_Transmit(&huart1,(uint8_t *)cmd,strlen(cmd),HAL_MAX_DELAY);
    HAL_UART_Transmit(&huart1,(uint8_t *)"\r\n",2,HAL_MAX_DELAY);
}

/**
  * @brief  从ESP8266的接收环形缓冲区中读取数据
  * @param  rb: 指向用于存储读出数据的缓冲区的指针
  * @param  len: 缓冲区的大小
  * @retval 实际读取到的字节数
  */
uint16_t ESP8266_Read(char *rb,uint16_t len)
{
    uint16_t bytes = 0;
    uint8_t data;
    while(bytes < len - 1)
    {
        if(Buffer_Read(&ESP8266_Rxbuf,&data) == Work_Ok)
        {
            rb[bytes++] = data;
        }else
        {
            break;
        }
    }
    rb[bytes] = '\0';
    return bytes;
}

/**
  * @brief  ESP8266发送命令并等待指定响应
  * @param  cmd: 要发送的AT命令字符串
  * @param  expected_response: 期望收到的响应字符串
  * @param  timeout: 等待超时时间（毫秒）
  * @retval 成功收到预期响应返回 Work_Ok，超时则返回 Error
  */
static ESP8266_Status ESP8266_SendCmdAndWait(const char *cmd,const char *expected_response,uint32_t timeout)
{
    Buffer_Clear(&ESP8266_Rxbuf);
    ESP8266_SendCmd(cmd);

    char rx_str[256] = {0};
    uint16_t rx_index = 0;

    TickType_t start_time = xTaskGetTickCount();
    TickType_t timeout_tick = pdMS_TO_TICKS(timeout);

    while(xTaskGetTickCount() - start_time <timeout_tick)
    {
        if(xSemaphoreTake(Esp_RxSem,pdMS_TO_TICKS(50)) == pdTRUE)
        {
            uint8_t data;
            while (Buffer_Read(&ESP8266_Rxbuf,&data) == Work_Ok)
            {
                if(rx_index < sizeof(rx_str) - 1)
                {
                    rx_str[rx_index++] = data;
                    rx_str[rx_index] = '\0';
                }

                if(strstr(rx_str,expected_response) != NULL)
                {
                    return Work_Ok;
                }
            }
            
        }
    }
    return Error;
}

/************通过MQTT连接Onenet云平台***********/
/**
  * @brief  检查是否收到指定的MQTT属性设置消息
  * @note   从ESP8266接收缓冲区读取数据，查找指定的OneNet属性设置主题，
  *         并在JSON数据中查找指定的参数名和参数值
  * @param  param: 要查找的参数名称
  * @param  value: 要查找的参数值
  * @retval ESP8266_Status: Work_Ok表示找到匹配，Error表示未找到或出错
  */
ESP8266_Status MQTT_Get(const char *param,const char *value)
{
    char *json;
    char json_id[12] = {0};
    char rx_str[512] = {0};
    if(ESP8266_Read(rx_str,sizeof(rx_str)) == 0)
    {
        return Error;
    }
    json = strstr((const char*)rx_str,ONENET_Set);

    if(json != NULL)
    {
        if(strchr(json,'{') != NULL)
        {
            strncpy(json_id,strchr(json,'{'),11);
        }else
        {
            return Error;
        }
        json += strlen(ONENET_Set);
        if(strstr(json,param) != NULL && strstr(json,value) != NULL)
        {
            return Work_Ok;
        }else
        {
            return Error;
        }
        
    }else
    {
        return Error;
    }
}

/**
  * @brief  ESP8266 MQTT 客户端初始化函数
  * @note   该函数依次执行以下步骤初始化ESP8266并连接到MQTT服务器：
  *         1. 复位模块
  *         2. 设置为STA模式
  *         3. 启用DHCP
  *         4. 连接Wi-Fi
  *         5. 配置MQTT用户信息
  *         6. 连接MQTT服务器
  *         7. 订阅回复主题
  *         8. 订阅属性设置主题
  *         9. 发布初始状态消息
  * @param  无
  * @retval ESP8266_Status: 所有步骤成功返回Work_Ok，任何一步失败返回Error
  * @note   此函数需要根据实际返回状态修改，当前版本缺少返回值判断
  */
ESP8266_Status MQTT_Init(void)
{
    /* 1 复位指令 */
    if(ESP8266_SendCmdAndWait("AT+RST","OK",Esp_Timeout) != Work_Ok)return Error;
    /* 2 设置为station模式 */
    if(ESP8266_SendCmdAndWait("AT+CWMODE=1","OK",Esp_Timeout) != Work_Ok)return Error;
    /* 3 启动DHCP，自动获取地址 */
    if(ESP8266_SendCmdAndWait("AT+CWDHCP=1,1","OK",Esp_Timeout) != Work_Ok)return Error;
    /* 4 连接wifi */
    if(ESP8266_SendCmdAndWait("AT+CWJAP=\"" Wifi_Name "\",\"" Wifi_Word "\"","OK",Esp_Timeout) != Work_Ok)return Error;
    /* 5 配置MQTT用户信息：设备名称，产品id，Token */
    if(ESP8266_SendCmdAndWait("AT+MQTTUSERCFG=0,1,\"" Equipment_Name "\",\"" Product_ID "\",\"" MQTT_Token "\",0,0,\"\"","OK",Esp_Timeout) != Work_Ok)return Error;
    /* 6 建立MQTT连接，配置域名和端口号 */
    if(ESP8266_SendCmdAndWait("AT+MQTTCONN=\"" Host "\",1883,1","OK",Esp_Timeout) != Work_Ok)return Error;
    /* 7 订阅主题：用于接收服务器对客户端发布消息的回复 */
    if(ESP8266_SendCmdAndWait("AT+MQTTSUB=0,\"" ONENET_Reply "\",1","OK",Esp_Timeout) != Work_Ok)return Error;
    /* 8 订阅主题：用于接收服务器下发的属性设置命令 */
    if(ESP8266_SendCmdAndWait("AT+MQTTSUB=0,\"" ONENET_Set "\",1","OK",Esp_Timeout) != Work_Ok)return Error;
    /* 9 发布消息 */
    if(ESP8266_SendCmdAndWait("AT+MQTTPUB=0,\"" ESP8266_Post "\",\"{\\\"id\\\":\\\"123\\\"\\,\\\"params\\\":{\\\"fan\\\":{\\\"value\\\":0\\}}}\",0,0","success",Esp_Timeout) != Work_Ok)return Error;
    if(ESP8266_SendCmdAndWait("AT+MQTTPUB=0,\"" ESP8266_Post "\",\"{\\\"id\\\":\\\"123\\\"\\,\\\"params\\\":{\\\"led\\\":{\\\"value\\\":false\\}}}\",0,0","success",Esp_Timeout) != Work_Ok)return Error;

    return Work_Ok;
}

/**
  * @brief  通过MQTT发布属性消息到OneNet平台
  * @note   构造JSON格式的属性上报消息并通过MQTT发布
  * @param  param: 要上报的属性名（如"temp"、"humi"等）
  * @param  value: 要上报的属性值（如"25.5"、"true"、"false"等）
  * @retval ESP8266_Status: Work_Ok表示发布成功，Error表示发布失败
  */
ESP8266_Status MQTT_Post(const char *param,const char *value)
{
    char buffer[156];
    static uint8_t count = 0;
    ESP8266_Status status = Work_Ok;

    count++;
    
    snprintf(buffer,sizeof(buffer),"AT+MQTTPUB=0,\"" ESP8266_Post "\",\"{\\\"id\\\":\\\"%d\\\"\\,\\\"params\\\":{\\\"%s\\\":{\\\"value\\\":%s\\}}}\",0,0\r\n", count, param, value);

    if(xSemaphoreTake(Post_Mutex,2000) == pdTRUE)
    {
        if(ESP8266_SendCmdAndWait(buffer,"success",Esp_Timeout) != Work_Ok)
        {
            status = Error;
        }
        xSemaphoreGive(Post_Mutex);
    }else
    {
        status = Error;
    }
    return status;

}
