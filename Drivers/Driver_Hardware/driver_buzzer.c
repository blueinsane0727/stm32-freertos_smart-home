#include "stm32f1xx_hal.h"
#include "driver_buzzer.h"

void Buzzer_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};
	Buzzer_Clk;
	HAL_GPIO_WritePin(Buzzer_Port, Buzzer_Pin, GPIO_PIN_RESET);
	
	GPIO_InitStruct.Pin = Buzzer_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(Buzzer_Port, &GPIO_InitStruct);
}

void Buzzer_Control(Buzzer_Mode mode)
{
	if(Buzzer_On)
	{
		HAL_GPIO_WritePin(Buzzer_Port, Buzzer_Pin, GPIO_PIN_SET);
	}else
	{
		HAL_GPIO_WritePin(Buzzer_Port, Buzzer_Pin, GPIO_PIN_RESET);
	}
}
