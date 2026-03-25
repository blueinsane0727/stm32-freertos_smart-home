#ifndef _DRIVER_DHT11_H
#define _DRIVER_DHT11_H

/*宏定义*/
#define DHT11_Pin  GPIO_PIN_8
#define DHT11_Port GPIOA
#define DHT11_CLK __HAL_RCC_GPIOA_CLK_ENABLE()

/*定义DHT11数据类型*/
typedef struct{
	float humi;
	float temp;
}dht11_result;

typedef enum{
	DHT11_TRUE,
	DHT11_ERROR
}dht11_return;

void DHT11_Init(void);	//初始化函数
void DHT11_Start(void);	//发送起始信号
dht11_return DHT11_Check(void);	//检查DHT11响应
dht11_return DHT11_Read_Bit(uint8_t *bit_val);	//读取一位数据
dht11_return DHT11_Read_Byte(uint8_t *byte_val);	//读取一字节数据
dht11_return DHT11_Read_Result(dht11_result *result);	//DHT11温湿度检测结果

#endif
