#ifndef UI_SCREEN_H
#define UI_SCREEN_H
#include "app_state.h"
uint8_t UI_App(const App *a,uint32_t now);
uint8_t UI_TempAlarm(int16_t temperature,uint8_t valid);
uint8_t UI_LightAlarm(uint8_t light);
uint8_t UI_SensorFault(uint8_t faults);
#endif
