#ifndef SENSORS_H
#define SENSORS_H
#include <stdint.h>
typedef struct {
    uint16_t light_raw, temp_raw;
    uint32_t sampled_at;
    uint8_t light_percent, light_valid, temp_valid;
    int16_t temp_tenths_c;
} SensorReadings;
void Sensors_Init(void);
uint8_t Sensors_Process(void);
const SensorReadings *Sensors_Get(void);
#endif
