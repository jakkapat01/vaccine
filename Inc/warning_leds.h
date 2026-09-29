#ifndef WARNING_LEDS_H
#define WARNING_LEDS_H
#include <stdint.h>
void WarningLEDs_Init(void);
void WarningLEDs_Process(uint8_t warning,uint32_t now);
#endif
