#include "app_constants.h"
#define ADC_POLL_TIMEOUT_MS 5U
#include "main.h"
#include "sensors.h"
#include "sensor_config.h"
#include "sensor_limits.h"
#include <math.h>
#define LIGHT_FULL_PERCENT 100U
#define ADC_PERCENT_ROUNDING 2047U
#define ADC_MAX_CODE 4095U
static ADC_HandleTypeDef adc;
static SensorReadings readings;
static uint32_t last_sample;
static uint8_t ready;
static uint8_t sampled;
static SensorFaultState faults;
static uint32_t retry_at;
static int16_t applied_low = TEMP_DEFAULT_LOW_C;
static int16_t applied_high = TEMP_DEFAULT_HIGH_C;
static volatile uint8_t alarm_pending;
static volatile uint8_t alarm_active;
static volatile uint32_t alarm_count;

/* Only the light channel enables EOC IRQs. Temperature retains AWD IRQs.
 * Main schedules a bounded burst; ISR reads, averages and checks light limits. */
static volatile uint8_t light_transfer;
static volatile uint8_t light_done;
static volatile uint8_t light_error;
static volatile unsigned light_index;
static volatile uint32_t light_sum;
static volatile uint16_t light_result;
static volatile uint8_t light_percent;
static volatile uint8_t light_alarm_pending;
static volatile uint8_t light_alarm_active;
static volatile uint32_t light_eoc_count;
static volatile uint32_t light_alarm_count;
static int16_t light_low = LIGHT_PERCENT_MIN;
static int16_t light_high = LIGHT_PERCENT_MAX;

static void light_check_from_isr(uint8_t percent)
{
    int hysteresis = ((light_high - light_low) / HYSTERESIS_RANGE_DIVISOR);
    if (hysteresis > LIGHT_HYSTERESIS_MAX)
    {
        hysteresis = LIGHT_HYSTERESIS_MAX;
    }
    else
    {
        /* No action. */
    }
    if ((light_alarm_active == 0U) && (((int)percent < light_low) || ((int)percent > light_high)))
    {
        light_alarm_active = 1U;
        light_alarm_pending = 1U;
        light_alarm_count++;
    }
    else if ((light_alarm_active != 0U) && ((int)percent >= (light_low + hysteresis)) &&
             ((int)percent <= (light_high - hysteresis)))
    {
        light_alarm_active = 0U;
        light_alarm_pending = 0U;
    }
    else
    {
        /* Keep one alarm per excursion until the hysteresis band is reached. */
    }
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *h)
{
    if ((h != &adc) || (light_transfer == 0U))
    {
        return;
    }
    else
    {
        /* The callback is reached through the real ADC EOC IRQ handler. */
    }
    uint16_t raw = (uint16_t)HAL_ADC_GetValue(h);
    light_eoc_count++;
    if (light_index != 0U)
    {
        light_sum += raw;
    }
    else
    {
        /* Discard the first conversion after switching the analog input. */
    }
    if (light_index == SENSOR_AVERAGES)
    {
        light_result = (uint16_t)((light_sum + (SENSOR_AVERAGES / 2U)) / SENSOR_AVERAGES);
        unsigned level =
            ((((uint32_t)light_result * LIGHT_FULL_PERCENT) + ADC_PERCENT_ROUNDING) / ADC_MAX_CODE);
#if LIGHT_HIGH_IS_BRIGHT
        light_percent = (uint8_t)level;
#else
        light_percent = (uint8_t)(LIGHT_FULL_PERCENT - level);
#endif
        light_check_from_isr(light_percent);
    }
    else
    {
        /* The burst has not collected all eight usable samples yet. */
    }
    __DMB();
    light_done = 1U;
}

void HAL_ADC_ErrorCallback(ADC_HandleTypeDef *h)
{
    if ((h == &adc) && (light_transfer != 0U))
    {
        light_error = 1U;
        __DMB();
        light_done = 1U;
    }
    else
    {
        /* Temperature polling and SensorFault keep their existing checks. */
    }
}

uint8_t Sensors_TakeLightAlarm(void)
{
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    uint8_t pending = light_alarm_pending;
    light_alarm_pending = 0U;
    __set_PRIMASK(primask);
    return pending;
}

uint8_t Sensors_LightAlarmActive(void)
{
    return light_alarm_active;
}

uint32_t Sensors_LightEocCount(void)
{
    return light_eoc_count;
}

uint32_t Sensors_LightAlarmCount(void)
{
    return light_alarm_count;
}

static uint8_t sample_light_interrupt(uint16_t *value)
{
    const SensorLimits *limits = SensorLimits_Get();
    /* No EOC IRQ is enabled here: publish the pair before starting conversion. */
    if ((light_low != limits->light_low) || (light_high != limits->light_high))
    {
        light_low = limits->light_low;
        light_high = limits->light_high;
        light_alarm_active = 0U;
        light_alarm_pending = 0U;
    }
    else
    {
        /* No action. */
    }
    ADC_ChannelConfTypeDef cfg = {0U};
    cfg.Channel = LIGHT_ADC_CHANNEL;
    cfg.Rank = 1U;
    cfg.SamplingTime = ADC_SAMPLETIME_480CYCLES;
    if (HAL_ADC_ConfigChannel(&adc, &cfg) != HAL_OK)
    {
        return 0U;
    }
    else
    {
        /* No action. */
    }
    light_sum = 0U;
    light_transfer = 1U;
    for (unsigned i = 0U; i <= SENSOR_AVERAGES; i++)
    {
        light_index = i;
        light_done = 0U;
        light_error = 0U;
        uint32_t started_at = HAL_GetTick();
        if (HAL_ADC_Start_IT(&adc) != HAL_OK)
        {
            light_transfer = 0U;
            HAL_ADC_Stop_IT(&adc);
            return 0U;
        }
        else
        {
            /* No action. */
        }
        /* Wait for the ISR-owned completion flag, never poll ADC EOC here.
         * Interrupts stay enabled; timeout preserves the existing fault path. */
        while (light_done == 0U)
        {
            if ((uint32_t)(HAL_GetTick() - started_at) >= ADC_POLL_TIMEOUT_MS)
            {
                light_transfer = 0U;
                HAL_ADC_Stop_IT(&adc);
                return 0U;
            }
            else
            {
                /* Await ADC IRQ or the bounded timeout. */
            }
        }
        HAL_ADC_Stop_IT(&adc);
        if (light_error != 0U)
        {
            light_transfer = 0U;
            return 0U;
        }
        else
        {
            /* No action. */
        }
    }
    light_transfer = 0U;
    *value = light_result;
    return 1U;
}

static uint16_t raw_at_c(float c)
{
    float r = (NTC_R25_OHM * expf((NTC_BETA_K * ((1.0f / (c + 273.15f)) - (1.0f / 298.15f)))));
    float ratio;
#if NTC_TO_GROUND

    ratio = (r / (NTC_FIXED_OHM + r));

#else

    ratio = (NTC_FIXED_OHM / (NTC_FIXED_OHM + r));

#endif
    return (uint16_t)lroundf((4095.0f * ratio));
}

static uint8_t watchdog_init(void)
{
    ADC_AnalogWDGConfTypeDef w = {0U};
    uint16_t a = raw_at_c(SensorLimits_Get()->temp_low);
    uint16_t b = raw_at_c(SensorLimits_Get()->temp_high);
    w.WatchdogMode = ADC_ANALOGWATCHDOG_SINGLE_REG;
    w.Channel = TEMP_ADC_CHANNEL;
    w.ITMode = ENABLE;
    if (a < b)
    {
        w.LowThreshold = a;
    }
    else
    {
        w.LowThreshold = b;
    }
    if (a > b)
    {
        w.HighThreshold = a;
    }
    else
    {
        w.HighThreshold = b;
    }
    return (HAL_ADC_AnalogWDGConfig(&adc, &w) == HAL_OK);
}

void Sensors_ADC_IRQHandler(void)
{
    HAL_ADC_IRQHandler(&adc);
}

void HAL_ADC_LevelOutOfWindowCallback(ADC_HandleTypeDef *h)
{
    if (h != &adc)
    {
        return;
    }
    else
    {
        /* No action. */
    }
    /* One IRQ per excursion: no OLED, floating point, UART or delay here. */
    __HAL_ADC_DISABLE_IT(h, ADC_IT_AWD);
    alarm_active = 1U;
    alarm_pending = 1U;
    alarm_count++;
}

uint8_t Sensors_TakeTempAlarm(void)
{
    uint32_t p = __get_PRIMASK();
    __disable_irq();
    uint8_t value = alarm_pending;
    alarm_pending = 0U;
    __set_PRIMASK(p);
    return value;
}

uint8_t Sensors_TempAlarmActive(void)
{
    return alarm_active;
}

uint32_t Sensors_TempAlarmCount(void)
{
    return alarm_count;
}

uint8_t Sensors_GetFaults(uint32_t now)
{
    return SensorFault_WithAge(faults.mask, sampled, now, readings.sampled_at);
}

void Sensors_Init(void)
{
    retry_at = HAL_GetTick();
    sampled = 0U;
    GPIO_InitTypeDef gpio = {0U};
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_ADC1_CLK_ENABLE();
    gpio.Pin = SENSOR_GPIO_PINS;
    gpio.Mode = GPIO_MODE_ANALOG;
    gpio.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &gpio);
    adc.Instance = ADC1;
    adc.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
    adc.Init.Resolution = ADC_RESOLUTION_12B;
    adc.Init.ScanConvMode = DISABLE;
    adc.Init.ContinuousConvMode = DISABLE;
    adc.Init.DiscontinuousConvMode = DISABLE;
    adc.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
    adc.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    adc.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    adc.Init.NbrOfConversion = 1U;
    adc.Init.DMAContinuousRequests = DISABLE;
    adc.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
    alarm_active = 0U;
    alarm_pending = 0U;
    alarm_count = 0U;
    HAL_NVIC_DisableIRQ(ADC_IRQn);
    light_transfer = 0U;
    light_done = 0U;
    light_error = 0U;
    light_alarm_pending = 0U;
    light_alarm_active = 0U;
    light_eoc_count = 0U;
    light_alarm_count = 0U;
    ready = (HAL_ADC_Init(&adc) == HAL_OK);
    if (ready != 0U)
    {
        ready = watchdog_init();
    }
    else
    {
        /* No action. */
    }
    HAL_NVIC_ClearPendingIRQ(ADC_IRQn);
    HAL_NVIC_SetPriority(ADC_IRQn, ADC_IRQ_PRIORITY, 0U);
    HAL_NVIC_EnableIRQ(ADC_IRQn);
    last_sample = (HAL_GetTick() - SENSOR_PERIOD_MS);
}

static uint8_t sample(uint32_t channel, uint16_t *value)
{
    ADC_ChannelConfTypeDef cfg = {0U};
    cfg.Channel = channel;
    cfg.Rank = 1U;
    cfg.SamplingTime = ADC_SAMPLETIME_480CYCLES;
    if (HAL_ADC_ConfigChannel(&adc, &cfg) != HAL_OK)
    {
        return 0U;
    }
    else
    {
        /* No action. */
    }
    uint32_t sum = 0U;
    /* Discard first conversion after switching inputs. */
    for (unsigned i = 0U; (i <= SENSOR_AVERAGES); i++)
    {
        if (HAL_ADC_Start(&adc) != HAL_OK)
        {
            return 0U;
        }
        else
        {
            /* No action. */
        }
        if (HAL_ADC_PollForConversion(&adc, ADC_POLL_TIMEOUT_MS) != HAL_OK)
        {
            HAL_ADC_Stop(&adc);
            return 0U;
        }
        else
        {
            /* No action. */
        }
        uint16_t raw = (uint16_t)HAL_ADC_GetValue(&adc);
        HAL_ADC_Stop(&adc);
        if (i != 0U)
        {
            sum += raw;
        }
        else
        {
            /* No action. */
        }
    }
    *value = (uint16_t)((sum + (SENSOR_AVERAGES / 2U)) / SENSOR_AVERAGES);
    return 1U;
}

uint8_t Sensors_Process(void)
{
    uint32_t now = HAL_GetTick();
    if ((((uint32_t)(now - last_sample) < SENSOR_PERIOD_MS) &&
         (applied_low == SensorLimits_Get()->temp_low)) &&
        (applied_high == SensorLimits_Get()->temp_high) &&
        (light_low == SensorLimits_Get()->light_low) &&
        (light_high == SensorLimits_Get()->light_high))
    {
        return 0U;
    }
    else
    {
        /* No action. */
    }
    if ((ready == 0U) && ((uint32_t)(now - retry_at) >= HARDWARE_RETRY_MS))
    {
        Sensors_Init();
    }
    else
    {
        /* No action. */
    }
    const SensorLimits *limits = SensorLimits_Get();
    if ((ready != 0U) && ((applied_low != limits->temp_low) || (applied_high != limits->temp_high)))
    {
        HAL_NVIC_DisableIRQ(ADC_IRQn);
        HAL_ADC_Stop(&adc);
        CLEAR_BIT(adc.Instance->CR2, ADC_CR2_CONT);
        __HAL_ADC_CLEAR_FLAG(&adc, ADC_FLAG_AWD);
        HAL_NVIC_ClearPendingIRQ(ADC_IRQn);
        alarm_pending = 0U;
        alarm_active = 0U;
        ready = watchdog_init();
        applied_low = limits->temp_low;
        applied_high = limits->temp_high;
        HAL_NVIC_EnableIRQ(ADC_IRQn);
    }
    else
    {
        /* No action. */
    }
    last_sample = now;
    /* Pause background temperature conversion before sampling the light channel. */
    if (ready != 0U)
    {
        HAL_ADC_Stop(&adc);
        CLEAR_BIT(adc.Instance->CR2, ADC_CR2_CONT);
    }
    else
    {
        /* No action. */
    }
    readings.light_valid = ((ready != 0U) && (sample_light_interrupt(&readings.light_raw) != 0U));
    readings.temp_valid = ((ready != 0U) && (sample(TEMP_ADC_CHANNEL, &readings.temp_raw) != 0U));
    uint8_t temp_read = readings.temp_valid;
    uint8_t light_read = readings.light_valid;
    readings.sampled_at = HAL_GetTick();
    sampled = 1U;
    if (readings.light_valid != 0U)
    {
        readings.light_percent = light_percent;
    }
    else
    {
        /* No action. */
    }
    if (readings.temp_valid != 0U)
    {
        unsigned raw = readings.temp_raw;
        if ((raw <= TEMP_RAW_MIN_VALID) || (raw >= TEMP_RAW_MAX_VALID))
        {
            readings.temp_valid = 0U;
        }
        else
        {
            float ratio = ((float)raw / (4095.0f - (float)raw));
            float r;
#if NTC_TO_GROUND

            r = (NTC_FIXED_OHM * ratio);

#else

            r = (NTC_FIXED_OHM * (1.0f / ratio));

#endif
            float c =
                ((1.0f / ((1.0f / 298.15f) + (logf((r / NTC_R25_OHM)) / NTC_BETA_K))) - 273.15f);
            if (((isfinite(c) == 0) || (c < -40.0f)) || (c > 125.0f))
            {
                readings.temp_valid = 0U;
            }
            else
            {
                readings.temp_tenths_c = (int16_t)lroundf((c * 10.0f));
            }
        }
    }
    else
    {
        /* No action. */
    }
    if (ready != 0U)
    {
        float hyst = fminf(1.0f, ((limits->temp_high - limits->temp_low) / 4.0f));
        if (((((alarm_active != 0U) && (alarm_pending == 0U)) && (readings.temp_valid != 0U)) &&
             (readings.temp_tenths_c >= (int)((limits->temp_low + hyst) * DECIMAL_BASE))) &&
            (readings.temp_tenths_c <= (int)((limits->temp_high - hyst) * DECIMAL_BASE)))
        {
            alarm_active = 0U;
            __HAL_ADC_CLEAR_FLAG(&adc, ADC_FLAG_AWD);
            __HAL_ADC_ENABLE_IT(&adc, ADC_IT_AWD);
        }
        else
        {
            /* No action. */
        }
        /* Temperature is the last selected channel. Keep converting while the
         * main loop handles menus or blocking OLED transfers. EOC/OVR IRQs were disabled by
         * Stop_IT; only AWD remains enabled. */
        ADC_ChannelConfTypeDef cfg = {0U};
        cfg.Channel = TEMP_ADC_CHANNEL;
        cfg.Rank = 1U;
        cfg.SamplingTime = ADC_SAMPLETIME_480CYCLES;
        if (HAL_ADC_ConfigChannel(&adc, &cfg) == HAL_OK)
        {
            SET_BIT(adc.Instance->CR2, ADC_CR2_CONT);
            if (HAL_ADC_Start(&adc) != HAL_OK)
            {
                readings.temp_valid = 0U;
                temp_read = 0U;
            }
            else
            {
                /* No action. */
            }
        }
        else
        {
            readings.temp_valid = 0U;
            temp_read = 0U;
        }
    }
    else
    {
        /* No action. */
    }
    uint8_t fault = SensorFault_Update(&faults, ready, temp_read, readings.temp_raw,
                                       readings.temp_valid, light_read, readings.light_raw);
    if ((fault & (SF_ADC | SF_TEMP_MASK)) != 0)
    {
        readings.temp_valid = 0U;
    }
    else
    {
        /* No action. */
    }
    if ((fault & (SF_ADC | SF_LIGHT_MASK)) != 0)
    {
        readings.light_valid = 0U;
    }
    else
    {
        /* No action. */
    }
    return 1U;
}

const SensorReadings *Sensors_Get(void)
{
    return &readings;
}
