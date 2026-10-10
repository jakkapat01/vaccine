#include "app_constants.h"
#include "vial_model.h"
#include "app_policy.h"
#include "sensor_limits.h"

void Vials_Init(Vial v[VIAL_COUNT])
{
    for (unsigned i = 0U; (i < VIAL_COUNT); i++)
    {
        v[i].health = HEALTH_FULL;
        v[i].sensitivity = (1.0f + (0.5f * (float)i));
        v[i].status = VIAL_AVAILABLE;
    }
}

void Vials_Update(Vial v[VIAL_COUNT], float seconds, float t, uint8_t valid, uint8_t open,
                  uint8_t light)
{
    if (seconds <= 0)
    {
        return;
    }
    else
    {
        /* No action. */
    }
    float excursion = 0;
    if (valid != 0U)
    {
        if (t < SensorLimits_Get()->temp_low)
        {
            excursion = (SensorLimits_Get()->temp_low - t);
        }
        else
        {
            /* No action. */
        }
        if (t > SensorLimits_Get()->temp_high)
        {
            excursion = (t - SensorLimits_Get()->temp_high);
        }
        else
        {
            /* No action. */
        }
    }
    else
    {
        /* No action. */
    }
    /* Missing temperature is reported as unknown, not silently treated as safe.
       Known open-lid exposure still accumulates. No invented temperature. */
    float rate;
    if (open != 0U)
    {
        rate = ((excursion * HEALTH_TEMP_RATE) + (HEALTH_OPEN_RATE * (1.0f + (light / 100.0f))));
    }
    else
    {
        rate = ((excursion * HEALTH_TEMP_RATE) + 0);
    }
    for (unsigned i = 0U; (i < VIAL_COUNT); i++)
    {
        v[i].health -= ((rate * seconds) * v[i].sensitivity);
        if (v[i].health <= 0)
        {
            v[i].health = 0;
            v[i].status = VIAL_QUARANTINED;
        }
        else
        {
            /* No action. */
        }
    }
}

uint8_t Vial_Return(Vial *v)
{
    if ((v->status != VIAL_AVAILABLE) || (v->health <= 0))
    {
        return 0U;
    }
    else
    {
        /* No action. */
    }
    /* Classroom assumption: the same vial is returned; preserve its health. */
    return 1U;
}

void Vial_Refill(Vial *v)
{
    v->health = HEALTH_FULL;
    v->status = VIAL_AVAILABLE;
}
