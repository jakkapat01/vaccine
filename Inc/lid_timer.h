#ifndef LID_TIMER_H
#define LID_TIMER_H
#include <stdint.h>
void LidTimer_Init(void);
void LidTimer_Start(uint32_t seconds);
void LidTimer_Stop(void);
void LidTimer_IRQHandler(void);
uint8_t LidTimer_TakeExpired(void);
uint32_t LidTimer_Count(void);
#endif
