#ifndef _DRIVER_MOTOR_H
#define _DRIVER_MOTOR_H

extern TIM_HandleTypeDef htim2;

typedef enum{
	Off_Mode = 0,
	Low_Mode,
	Mid_Mode,
	Fast_Mode
}Motor_Mode;

typedef enum
{
	Fan_Auto = 0,
	Fan_Manual,
	Fan_Cloud
}Fan_ControlState;

#define Motor_Port 			GPIOA
#define Motor_PWM_Pin 		GPIO_PIN_1
#define Motor_AIN1_Pin 		GPIO_PIN_2
#define Motor_AIN2_Pin 		GPIO_PIN_3
#define Motor_CLK 			__HAL_RCC_GPIOA_CLK_ENABLE()
#define Motor_Channel		TIM_CHANNEL_2
#define Motor_Htim			&htim2

#define Low_Speed  300
#define Mid_Speed  600
#define Fast_Speed 900

/*初始化*/
void Fan_AppInit(void);
/*设置/获取当前风扇的挡位*/
void Fan_SetMode(Motor_Mode mode);
Motor_Mode Fan_GetMode(void);
/*设置/获取当前风扇的控制模式*/
void Fan_SetControlMode(Fan_ControlState mode);
Fan_ControlState Fan_GetControlMode(void);
/*切换到下一档*/
Motor_Mode Fan_GetNextMode(Motor_Mode mode);
void Fan_SwitchNextMode(void);

/*自动控制模式：根据温度切换挡位*/
Motor_Mode Fan_GetAutoState(float temp);
void Fan_AutoControl(float temp);

/*云端接口*/
void Fan_CloudSet(Motor_Mode mode);

#endif
