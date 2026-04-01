#include "stm32f1xx_hal.h"
#include "driver_motor.h"

static Motor_Mode fanstate = Off_Mode;
static Fan_ControlState fan_control = Fan_Auto;

/***********电机驱动层***********/
static void Motor_Init(void)
{
	HAL_GPIO_WritePin(Motor_Port, Motor_PWM_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(Motor_Port, Motor_AIN1_Pin, GPIO_PIN_RESET);	
	HAL_GPIO_WritePin(Motor_Port, Motor_AIN2_Pin, GPIO_PIN_RESET);	
	
	Motor_CLK;
	GPIO_InitTypeDef GPIO_InitStruct = {0};
	
	//配置电机的PWM引脚
	GPIO_InitStruct.Pin = Motor_PWM_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(Motor_Port, &GPIO_InitStruct);
	
	//配置输出引脚
	GPIO_InitStruct.Pin = Motor_AIN1_Pin | Motor_AIN2_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(Motor_Port, &GPIO_InitStruct);
}

static void Motor_SetMode(Motor_Mode mode)
{
	TIM_OC_InitTypeDef sConfigOC = {0};
	
	sConfigOC.OCMode = TIM_OCMODE_PWM1;
	sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
	sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
	
	switch(mode)
	{
		case Off_Mode :
			sConfigOC.Pulse = 0;
			HAL_GPIO_WritePin(Motor_Port, Motor_AIN1_Pin, GPIO_PIN_RESET);	
			HAL_GPIO_WritePin(Motor_Port, Motor_AIN2_Pin, GPIO_PIN_RESET);	
			break;
		
		case Low_Mode :
			sConfigOC.Pulse = Low_Speed;
			HAL_GPIO_WritePin(Motor_Port, Motor_AIN1_Pin, GPIO_PIN_SET);	
			HAL_GPIO_WritePin(Motor_Port, Motor_AIN2_Pin, GPIO_PIN_RESET);	
			break;
		
		case Mid_Mode :
			sConfigOC.Pulse = Mid_Speed;
			HAL_GPIO_WritePin(Motor_Port, Motor_AIN1_Pin, GPIO_PIN_SET);	
			HAL_GPIO_WritePin(Motor_Port, Motor_AIN2_Pin, GPIO_PIN_RESET);	
			break;
		
		case Fast_Mode :
			sConfigOC.Pulse = Fast_Speed;
			HAL_GPIO_WritePin(Motor_Port, Motor_AIN1_Pin, GPIO_PIN_SET);	
			HAL_GPIO_WritePin(Motor_Port, Motor_AIN2_Pin, GPIO_PIN_RESET);	
			break;
	}
	
	HAL_TIM_PWM_Stop(Motor_Htim,Motor_Channel);
	HAL_TIM_PWM_ConfigChannel(Motor_Htim,&sConfigOC,Motor_Channel);
	HAL_TIM_PWM_Start(Motor_Htim,Motor_Channel);
}

/***********风扇应用层***********/
void Fan_AppInit(void)
{
	Motor_Init();
	Motor_SetMode(Off_Mode);
	fanstate = Off_Mode;
	fan_control = Fan_Auto;
}

void Fan_SetMode(Motor_Mode mode)
{
	Motor_SetMode(mode);
	fanstate = mode;
}

Motor_Mode Fan_GetMode(void)
{
	return fan_control;
}

void Fan_SetControlMode(Fan_ControlState mode)
{
	fan_control = mode;
}

Fan_ControlState Fan_GetControlMode(void)
{
	return fan_control;
}

Motor_Mode Fan_GetNextMode(Motor_Mode mode)
{
	switch (mode)
	{
	case Off_Mode:
		return Low_Mode;

	case Low_Mode:
		return Mid_Mode;
	
	case Mid_Mode:
		return Fast_Mode;
	
	default:
		return Off_Mode;
	}
}

void Fan_SwitchNextMode(void)
{
	Motor_Mode nextstate = Fan_GetNextMode(fanstate);
	Fan_SetMode(nextstate);
}

Motor_Mode Fan_GetAutoState(float temp)
{
	if(temp < 25.0f)
	{
		return Off_Mode;
	}
	else if(temp < 30.0f)
	{
		return Low_Mode;
	}
	else if (temp < 32.0f)
	{
		return Mid_Mode;
	}
	else
	{
		return Fast_Mode;
	}
	
}

void Fan_AutoControl(float temp)
{
	if(fan_control == Fan_Auto)
	{
		Fan_SetMode(Fan_GetAutoState(temp));
	}
}

void Fan_CloudSet(Motor_Mode mode)
{
	fan_control = Fan_Cloud;
	Fan_SetMode(mode);
}
