#ifndef SENSORS_H
#define SENSORS_H
#include <stdint.h>
#include "sensor_fault.h"
typedef struct {
    uint16_t light_raw, temp_raw;
    uint32_t sampled_at;
    uint8_t light_percent, light_valid, temp_valid;
    int16_t temp_tenths_c;
} SensorReadings;
void Sensors_Init(void);
uint8_t Sensors_Process(void);
const SensorReadings *Sensors_Get(void);
void Sensors_ADC_IRQHandler(void);
uint8_t Sensors_TakeTempAlarm(void);
uint8_t Sensors_TempAlarmActive(void);
uint32_t Sensors_TempAlarmCount(void);
uint8_t Sensors_GetFaults(uint32_t now);
#endif
