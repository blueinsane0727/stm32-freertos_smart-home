#ifndef _DRIVER_LED_H
#define _DRIVER_LED_H

/*LED使用的宏定义*/
#define Led_Port GPIOC
#define Led_Pin GPIO_PIN_13
#define Led_CLK __HAL_RCC_GPIOC_CLK_ENABLE()

void Led_Init(void);	//Led初始化
void Led_On(void);	//开灯
void Led_Off(void);	//关灯
void Led_Turn(void);	//改变状态

#endif
