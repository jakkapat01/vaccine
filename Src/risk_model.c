#include "app_constants.h"
#define RISK_MAX_PERCENT 100U
#include "risk_model.h"
#include "app_policy.h"
#include "sensor_limits.h"

uint8_t Risk_Calculate(float t, float outside, const Vial *v, uint8_t valid)
{
    if (valid == 0U)
    {
        return RISK_MAX_PERCENT;
    }
    else
    {
        /* No action. */
    }
    float excursion;
    if (t < SensorLimits_Get()->temp_low)
    {
        excursion = (SensorLimits_Get()->temp_low - t);
    }
    else
    {
        if (t > SensorLimits_Get()->temp_high)
        {
            excursion = (t - SensorLimits_Get()->temp_high);
        }
        else
        {
            excursion = 0;
        }
    }
    float warm;
    if (outside > t)
    {
        warm = (outside - t);
    }
    else
    {
        warm = 0;
    }
    float score = (((excursion * MODEL_TEMP_RISK_WEIGHT) + (warm * MODEL_OUTSIDE_RISK_WEIGHT)) +
                   ((HEALTH_FULL - v->health) * MODEL_HEALTH_RISK_WEIGHT));
    if (score > HEALTH_FULL)
    {
        score = HEALTH_FULL;
    }
    else
    {
        /* No action. */
    }
    if (score < 0)
    {
        score = 0;
    }
    else
    {
        /* No action. */
    }
    return (uint8_t)(score + 0.5f);
}
