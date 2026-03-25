#include "stm32f1xx_hal.h"
#include "driver_light_sensor.h"

static ADC_HandleTypeDef *Hadc_Sensor = &hadc1;

void LightSensor_Init(void)
{
	HAL_ADCEx_Calibration_Start(Hadc_Sensor);
}

int LightSensor_Read(light_data *pdata)
{
	HAL_ADC_Start(Hadc_Sensor);
	int adc_max = 4095;
	int adc_value = 0;
	
	if (HAL_OK == HAL_ADC_PollForConversion(Hadc_Sensor,LightSensor_Timeout))
	{
		adc_value = HAL_ADC_GetValue(Hadc_Sensor);
		pdata->data = (1 - ((float)adc_value / adc_max)) * 100;
		return Light_OK;
	}else
	{
		return Light_ERR;
	}
}
