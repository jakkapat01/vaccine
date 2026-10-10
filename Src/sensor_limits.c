#include "app_constants.h"
#include "sensor_limits.h"
static SensorLimits limits = {TEMP_DEFAULT_LOW_C, TEMP_DEFAULT_HIGH_C, LIGHT_PERCENT_MIN,
                              LIGHT_PERCENT_MAX};

const SensorLimits *SensorLimits_Get(void)
{
    return &limits;
}

uint8_t SensorLimits_Set(uint8_t light, int low, int high)
{
    int minimum = CONFIG_TEMP_MIN_C;
    int maximum = CONFIG_TEMP_MAX_C;
    if (light != 0U)
    {
        minimum = 0;
        maximum = LIGHT_PERCENT_MAX;
    }
    else
    {
        /* No action. */
    }
    if (((low >= high) || (low < minimum)) || (high > maximum))
    {
        return 0U;
    }
    else
    {
        /* No action. */
    }
    if (light != 0U)
    {
        limits.light_low = (int16_t)low;
        limits.light_high = (int16_t)high;
    }
    else
    {
        limits.temp_low = (int16_t)low;
        limits.temp_high = (int16_t)high;
    }
    return 1U;
}
