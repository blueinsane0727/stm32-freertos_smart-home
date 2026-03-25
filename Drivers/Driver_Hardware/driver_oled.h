#ifndef _DRIVER_OLED_H
#define _DRIVER_OLED_H

#include <stdint.h>

void OLED_Init(void);                                                       
void OLED_SetPosition(uint8_t page, uint8_t col);                           
void OLED_Clear(void);
void OLED_PutChar(uint8_t x, uint8_t y, char c);
int OLED_PrintString(uint8_t x, uint8_t y, const char *str);
void OLED_ClearLine(uint8_t x, uint8_t y);
int OLED_PrintHex(uint8_t x, uint8_t y, uint32_t val, uint8_t pre);
int OLED_PrintSignedVal(uint8_t x, uint8_t y, int32_t val);
void OLED_PrintFloat(uint8_t x,uint8_t y,float val,uint8_t accuracy);

#endif 
