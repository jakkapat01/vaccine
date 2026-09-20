#ifndef OLED_DISPLAY_H
#define OLED_DISPLAY_H
#include "main.h"
uint8_t OLED_Init(void);
void OLED_Clear(void);
void OLED_Text(uint8_t x,uint8_t y,const char *text,uint8_t scale);
void OLED_Rect(unsigned x,unsigned y,unsigned w,unsigned h,uint8_t filled);
uint8_t OLED_Update(void);
#endif
