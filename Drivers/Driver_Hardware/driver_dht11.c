#include "stm32f1xx_hal.h"
#include "driver_dht11.h"

/*DHT11使用的us级别延时函数*/
static void Delay_us(int us)
{
    extern TIM_HandleTypeDef        htim4;
    TIM_HandleTypeDef *hHalTim = &htim4;

    uint32_t ticks;
    uint32_t told, tnow, tcnt = 0;
    uint32_t reload = __HAL_TIM_GET_AUTORELOAD(hHalTim);

    ticks = us * reload / (1000); 
    told = __HAL_TIM_GET_COUNTER(hHalTim);
    while (1)
    {
        tnow = __HAL_TIM_GET_COUNTER(hHalTim);
        if (tnow != told)
        {
            if (tnow > told)
            {
                tcnt += tnow - told;
            }
            else
            {
                tcnt += reload - told + tnow;
            }
            told = tnow;
            if (tcnt >= ticks)
            {
                break;
            }
        }
    }

}

/*将DHT11引脚设置为输入模式*/
static void DHT11_Set_Input(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = DHT11_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP; 
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(DHT11_Port, &GPIO_InitStruct);
}

/*将DHT11引脚设置为输出模式*/
static void DHT11_Set_Output(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = DHT11_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(DHT11_Port, &GPIO_InitStruct);
}

/* 函数名称：DHT11_Init
** 输入参数：无
** 返回值：无
** 函数功能：开启时钟，设置为输出模式，初始化为高电平
*/
void DHT11_Init(void)
{
	
	DHT11_CLK;
	DHT11_Set_Output();
	HAL_GPIO_WritePin(DHT11_Port, DHT11_Pin, GPIO_PIN_SET);	
	
}

/* 函数名称：DHT11_Start
** 输入参数：无
** 返回值：无
** 函数功能：发出起始信号，等待DHT11响应
*/
void DHT11_Start(void)
{
	DHT11_Init();
	HAL_GPIO_WritePin(DHT11_Port, DHT11_Pin, GPIO_PIN_RESET);
	HAL_Delay(20);
	HAL_GPIO_WritePin(DHT11_Port, DHT11_Pin, GPIO_PIN_SET);
	Delay_us(20);
}

/* 函数名称：DHT11_Check
** 输入参数：无
** 返回值：dht11_return类型枚举值
**         DHT11_ERROR 错误信号，可能连接有误
**		   DHT11_TRUE 正确信号，表示DHT11正常工作
** 函数功能：检查DHT11响应信号是否正常
*/
dht11_return DHT11_Check(void)
{
	uint16_t time = 0;
	DHT11_Set_Input();
	while(HAL_GPIO_ReadPin(DHT11_Port, DHT11_Pin) && time < 100)
	{
		time++;
		Delay_us(1);
	}
	if(time >= 100)
	{
		return DHT11_ERROR;
	}else
	{
		time = 0;
		
	}
	while(!HAL_GPIO_ReadPin(DHT11_Port, DHT11_Pin) && time < 100)
	{
		time++;
		Delay_us(1);
	}
	if(time >= 100)
	{
		return DHT11_ERROR;
	}else
	{
		return DHT11_TRUE;
	}
}

/* 函数名称：DHT11_Read_Bit
** 输入参数：uint8_t *bit_val 将读取到的bit值存放在该指针指向的地址
** 返回值：dht11_return类型枚举值
**         DHT11_ERROR 错误信号，可能连接有误
**		   DHT11_TRUE 正确信号，表示DHT11正常工作
** 函数功能：读取一位数据，返回操作是否成功
*/
dht11_return DHT11_Read_Bit(uint8_t *bit_val)
{
	uint16_t time = 0;
	while(HAL_GPIO_ReadPin(DHT11_Port, DHT11_Pin) && time < 100)
	{
		time++;
		Delay_us(1);
	}
	if(time >= 100)
	{
		return DHT11_ERROR;
	}else
	{
		time = 0;
		
	}
	while(!HAL_GPIO_ReadPin(DHT11_Port, DHT11_Pin) && time < 100)
	{
		time++;
		Delay_us(1);
	}
	if(time >= 100)
	{
		return DHT11_ERROR;
	}
	Delay_us(40);
	if(HAL_GPIO_ReadPin(DHT11_Port, DHT11_Pin))
	{
		*bit_val = 1;
	}else
	{
		*bit_val = 0;
	}

	return DHT11_TRUE;
}

/* 函数名称：DHT11_Read_Byte
** 输入参数：uint8_t *byte_val 将读取到的一字节数据存储在该指针指向的地址
** 返回值：dht11_return类型枚举值
**         DHT11_ERROR 错误信号，可能连接有误
**		   DHT11_TRUE 正确信号，表示DHT11正常工作
** 函数功能：读取一字节数据，返回操作是否成功
*/
dht11_return DHT11_Read_Byte(uint8_t *byte_val)
{
	uint8_t i,data,bit_val;
	data = 0; 
	for(i = 0;i < 8;i++)
	{
		if(DHT11_Read_Bit(&bit_val) == DHT11_ERROR)
		{
			return DHT11_ERROR;
		}

		data<<=1;
		data |= bit_val;
	}
	*byte_val = data;
	return DHT11_TRUE;			
}

/* 函数名称：DHT11_Read_Result
** 输入参数：dht11_result类型的数据指针
** 返回值：dht11_return类型枚举值
**         DHT11_ERROR 错误信号，可能连接有误
**		   DHT11_TRUE 正确信号，表示DHT11正常工作
** 函数功能：读取一次温湿度数据存放在结构体里，返回操作是否成功
*/
dht11_return DHT11_Read_Result(dht11_result *result)
{
	uint8_t i;
	uint8_t buf[5];
	uint16_t sum;
	result->humi = 0;
	result->temp = 0;
	DHT11_Start();
	if(DHT11_Check() == DHT11_TRUE)
	{
		for(i=0;i<5;i++)
		{
			if(DHT11_Read_Byte(&buf[i]) == DHT11_ERROR)
			{
				return DHT11_ERROR;
			}
		}
		sum = (uint16_t)buf[0] + (uint16_t)buf[1] + (uint16_t)buf[2] + (uint16_t)buf[3];
		if(buf[4] == (uint8_t)sum)
		{
			result->humi = buf[0] + buf[1]/10.0;
			result->temp = buf[2] + buf[3]/10.0;
			return DHT11_TRUE;
		}else
		{
			return DHT11_ERROR;
		}
	 }else
	{
		return DHT11_ERROR;
	}
	
}
