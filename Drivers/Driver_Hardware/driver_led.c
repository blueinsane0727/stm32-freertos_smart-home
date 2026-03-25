#include "stm32f1xx_hal.h"
#include "driver_led.h"

/* 函数名称：Led_Init
** 输入参数：无
** 返回值：无
** 函数功能：初始化LED引脚
*/
void Led_Init(void)
{
	Led_CLK;
	GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = Led_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(Led_Port, &GPIO_InitStruct);
	
	HAL_GPIO_WritePin(Led_Port, Led_Pin, GPIO_PIN_RESET);
}

/* 函数名称：Led_On
** 输入参数：无
** 返回值：无
** 函数功能：开灯
*/
void Led_On(void)
{
	HAL_GPIO_WritePin(Led_Port, Led_Pin, GPIO_PIN_SET);
}

/* 函数名称：Led_Off
** 输入参数：无
** 返回值：无
** 函数功能：关灯
*/
void Led_Off(void)
{
	HAL_GPIO_WritePin(Led_Port, Led_Pin, GPIO_PIN_RESET);
}

/* 函数名称：Led_Turn
** 输入参数：无
** 返回值：无
** 函数功能：改变LED状态，灯亮时关灯，灯不亮时开灯
*/
void Led_Turn(void)
{
	HAL_GPIO_TogglePin(Led_Port, Led_Pin);
}
