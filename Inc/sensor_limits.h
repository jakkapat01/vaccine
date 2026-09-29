#ifndef SENSOR_LIMITS_H
#define SENSOR_LIMITS_H
#include <stdint.h>
typedef struct {int16_t temp_low,temp_high,light_low,light_high;} SensorLimits;
const SensorLimits *SensorLimits_Get(void);
uint8_t SensorLimits_Set(uint8_t light,int low,int high);
#endif
