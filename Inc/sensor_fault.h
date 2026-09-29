#ifndef SENSOR_FAULT_H
#define SENSOR_FAULT_H
#include <stdint.h>
enum {SF_ADC=1U,SF_TEMP_READ=2U,SF_TEMP_RAIL=4U,SF_TEMP_RANGE=8U,
      SF_LIGHT_READ=16U,SF_LIGHT_RAIL=32U,SF_STALE=64U};
#define SF_TEMP_MASK (SF_TEMP_READ|SF_TEMP_RAIL|SF_TEMP_RANGE)
#define SF_LIGHT_MASK (SF_LIGHT_READ|SF_LIGHT_RAIL)
typedef struct {uint8_t mask,temp_good,light_good,light_rail;} SensorFaultState;
uint8_t SensorFault_Update(SensorFaultState *f,uint8_t ready,uint8_t temp_read,
    uint16_t temp_raw,uint8_t temp_valid,uint8_t light_read,uint16_t light_raw);
uint8_t SensorFault_WithAge(uint8_t mask,uint8_t sampled,uint32_t now,uint32_t at);
#endif
