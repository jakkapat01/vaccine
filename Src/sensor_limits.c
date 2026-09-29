#include "sensor_limits.h"
static SensorLimits limits={2,8,0,100};
const SensorLimits *SensorLimits_Get(void) {return &limits;}
uint8_t SensorLimits_Set(uint8_t light,int low,int high) {
    if(low>=high || low<(light ? 0:-40) || high>(light ? 100:125)) return 0;
    if(light) {limits.light_low=low;limits.light_high=high;}
    else {limits.temp_low=low;limits.temp_high=high;}
    return 1;
}
