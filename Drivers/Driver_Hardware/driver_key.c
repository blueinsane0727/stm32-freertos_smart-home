#include "stm32f1xx_hal.h"
#include "driver_key.h"
#include "driver_led.h"
#include "driver_motor.h"

static Motor_Mode mode = Off_Mode;

/* 函数名称：Key_Init
** 输入参数：无
** 返回值：无
** 函数功能：初始化按键引脚
*/
void Key_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = Key1_Pin | Key2_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(Key_Port, &GPIO_InitStruct);
	
	__HAL_RCC_AFIO_CLK_ENABLE();
	HAL_NVIC_SetPriority(EXTI15_10_IRQn, 5, 0);
	HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
}

void Key_Fan(void)
{
	switch(mode)
	{
		case Off_Mode:
			Fan_SetMode(Off_Mode);
			break;
		
		case Low_Mode:
			Fan_SetMode(Low_Mode);
			break;
		
		case Mid_Mode:
			Fan_SetMode(Mid_Mode);
			break;
		
		case Fast_Mode:
			Fan_SetMode(Fast_Mode);
			break;
		
	}
}



