#ifndef _DRIVER_BUZZER_H
#define _DRIVER_BUZZER_H

#define Buzzer_Port		GPIOB
#define Buzzer_Pin		GPIO_PIN_15
#define Buzzer_Clk      __HAL_RCC_GPIOB_CLK_ENABLE()

typedef enum
{
	Buzzer_Off = 0,
	Buzzer_On
}Buzzer_Mode;

void Buzzer_Init(void);
void Buzzer_Control(Buzzer_Mode mode);

#endif
