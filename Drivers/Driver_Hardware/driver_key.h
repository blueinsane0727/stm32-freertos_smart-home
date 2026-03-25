#ifndef _DRIVER_KEY_H
#define _DRIVER_KEY_H

/*宏定义：定义两个按键引脚*/
#define Key_Port GPIOC
#define Key1_Pin GPIO_PIN_15
#define Key2_Pin GPIO_PIN_14

void Key_Init(void);	//按键初始化
void Key_Fan(void);

#endif
