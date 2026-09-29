#ifndef BUTTONS_H
#define BUTTONS_H
#include <stdint.h>
void Buttons_Init(void);
uint8_t Buttons_GetEvent(void);
uint8_t Buttons_TakeOpenRequest(void);
void Buttons_Edge(uint16_t pin);
void Buttons_GetIrqStats(uint32_t counts[4],uint32_t *lost);
#endif
