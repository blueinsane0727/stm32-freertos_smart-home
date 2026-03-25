#include "stm32f1xx_hal.h"
#include "stm32f103xb.h"                  

void CLK_Enable(GPIO_TypeDef GPIO)
{
	switch (GPIO)
	{
		case GPIOA:
			__HAL_RCC_GPIOA_CLK_ENABLE();
			break;
		
		case GPIOB:
			__HAL_RCC_GPIOB_CLK_ENABLE();
			break;
		
		case GPIOC:
			__HAL_RCC_GPIOC_CLK_ENABLE();
			break;
		
	}
}
