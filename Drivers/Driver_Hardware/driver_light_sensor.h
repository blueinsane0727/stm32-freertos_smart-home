#ifndef _DRIVER_LIGHT_SENSOR_H
#define _DRIVER_LIGHT_SENSOR_H

extern ADC_HandleTypeDef hadc1;

typedef struct 
{
    int data;
}light_data;

#define LightSensor_Timeout  500
#define Light_OK 			 0
#define Light_ERR			 -1

void LightSensor_Init(void);
int LightSensor_Read(light_data *pdata);

#endif
