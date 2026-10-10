#include "app_constants.h"
#include "sensor_fault.h"
#include "app_policy.h"

static uint8_t next(uint8_t n)
{
    if (n < FAULT_CONFIRM_SAMPLES)
    {
        return (n + 1U);
    }
    else
    {
        return FAULT_CONFIRM_SAMPLES;
    }
}

uint8_t SensorFault_Update(SensorFaultState *f, uint8_t ready, uint8_t tr, uint16_t raw, uint8_t tv,
                           uint8_t lr, uint16_t light)
{
    uint8_t temp = 0U;
    uint8_t illum = 0U;
    if (ready == 0U)
    {
        f->mask = SF_ADC;
        f->light_good = 0U;
        f->temp_good = 0U;
        return f->mask;
    }
    else
    {
        /* No action. */
    }
    f->mask &= (uint8_t)~SF_ADC;
    if (tr == 0U)
    {
        temp = SF_TEMP_READ;
    }
    else if ((raw <= TEMP_RAW_MIN_VALID) || (raw >= TEMP_RAW_MAX_VALID))
    {
        temp = SF_TEMP_RAIL;
    }
    else if (tv == 0U)
    {
        temp = SF_TEMP_RANGE;
    }
    else
    {
        /* No action. */
    }
    if (lr == 0U)
    {
        illum = SF_LIGHT_READ;
        f->light_rail = 0U;
    }
    else if ((light <= LIGHT_RAW_MIN_VALID) || (light >= LIGHT_RAW_MAX_VALID))
    {
        f->light_rail = next(f->light_rail);
        if (f->light_rail >= FAULT_CONFIRM_SAMPLES)
        {
            illum = SF_LIGHT_RAIL;
        }
        else
        {
            /* No action. */
        }
    }
    else
    {
        f->light_rail = 0U;
    }
    if (temp != 0U)
    {
        f->mask = ((f->mask & (uint8_t)~SF_TEMP_MASK) | temp);
        f->temp_good = 0U;
    }
    else if ((f->temp_good = next(f->temp_good)) >= FAULT_CONFIRM_SAMPLES)
    {
        f->mask &= (uint8_t)~SF_TEMP_MASK;
    }
    else
    {
        /* No action. */
    }
    if (illum != 0U)
    {
        f->mask = ((f->mask & (uint8_t)~SF_LIGHT_MASK) | illum);
        f->light_good = 0U;
    }
    else if ((f->light_rail == 0U) &&
             ((f->light_good = next(f->light_good)) >= FAULT_CONFIRM_SAMPLES))
    {
        f->mask &= (uint8_t)~SF_LIGHT_MASK;
    }
    else
    {
        /* No action. */
    }
    return f->mask;
}

uint8_t SensorFault_WithAge(uint8_t mask, uint8_t sampled, uint32_t now, uint32_t at)
{
    if ((sampled == 0U) || ((uint32_t)(now - at) >= SENSOR_STALE_MS))
    {
        return (mask | SF_STALE);
    }
    else
    {
        return (mask | 0U);
    }
}
