#include "main.h"
#include "sensors.h"
#include "sensor_config.h"
#include "sensor_limits.h"
#include <math.h>
static ADC_HandleTypeDef adc;
static SensorReadings readings;
static uint32_t last_sample;
static uint8_t ready,sampled;
static SensorFaultState faults;
static uint32_t retry_at;
static int16_t applied_low=2,applied_high=8;
static volatile uint8_t alarm_pending,alarm_active;
static volatile uint32_t alarm_count;
static uint16_t raw_at_c(float c) {
    float r=NTC_R25_OHM*expf(NTC_BETA_K*(1.0f/(c+273.15f)-1.0f/298.15f));
    float ratio=NTC_TO_GROUND ? r/(NTC_FIXED_OHM+r):NTC_FIXED_OHM/(NTC_FIXED_OHM+r);
    return (uint16_t)lroundf(4095.0f*ratio);
}
static uint8_t watchdog_init(void) {
    ADC_AnalogWDGConfTypeDef w={0};
    uint16_t a=raw_at_c(SensorLimits_Get()->temp_low),b=raw_at_c(SensorLimits_Get()->temp_high);
    w.WatchdogMode=ADC_ANALOGWATCHDOG_SINGLE_REG;
    w.Channel=TEMP_ADC_CHANNEL;w.ITMode=ENABLE;
    w.LowThreshold=a<b ? a:b;w.HighThreshold=a>b ? a:b;
    return HAL_ADC_AnalogWDGConfig(&adc,&w)==HAL_OK;
}
void Sensors_ADC_IRQHandler(void) {HAL_ADC_IRQHandler(&adc);}
void HAL_ADC_LevelOutOfWindowCallback(ADC_HandleTypeDef *h) {
    if(h!=&adc) return;
    /* One IRQ per excursion: no OLED, floating point, UART or delay here. */
    __HAL_ADC_DISABLE_IT(h,ADC_IT_AWD);
    alarm_active=1;alarm_pending=1;alarm_count++;
}
uint8_t Sensors_TakeTempAlarm(void) {
    uint32_t p=__get_PRIMASK();__disable_irq();
    uint8_t value=alarm_pending;alarm_pending=0;__set_PRIMASK(p);return value;
}
uint8_t Sensors_TempAlarmActive(void) {return alarm_active;}
uint32_t Sensors_TempAlarmCount(void) {return alarm_count;}
uint8_t Sensors_GetFaults(uint32_t now) {return SensorFault_WithAge(faults.mask,sampled,now,readings.sampled_at);}


void Sensors_Init(void)
{
    retry_at=HAL_GetTick();sampled=0;
    GPIO_InitTypeDef gpio={0};
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_ADC1_CLK_ENABLE();
    gpio.Pin=SENSOR_GPIO_PINS;
    gpio.Mode=GPIO_MODE_ANALOG;
    gpio.Pull=GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA,&gpio);
    adc.Instance=ADC1;
    adc.Init.ClockPrescaler=ADC_CLOCK_SYNC_PCLK_DIV4;
    adc.Init.Resolution=ADC_RESOLUTION_12B;
    adc.Init.ScanConvMode=DISABLE;
    adc.Init.ContinuousConvMode=DISABLE;
    adc.Init.DiscontinuousConvMode=DISABLE;
    adc.Init.ExternalTrigConvEdge=ADC_EXTERNALTRIGCONVEDGE_NONE;
    adc.Init.ExternalTrigConv=ADC_SOFTWARE_START;
    adc.Init.DataAlign=ADC_DATAALIGN_RIGHT;
    adc.Init.NbrOfConversion=1;
    adc.Init.DMAContinuousRequests=DISABLE;
    adc.Init.EOCSelection=ADC_EOC_SINGLE_CONV;
    alarm_pending=alarm_active=0;alarm_count=0;
    HAL_NVIC_DisableIRQ(ADC_IRQn);
    ready=(HAL_ADC_Init(&adc)==HAL_OK);
    if(ready) ready=watchdog_init();
    HAL_NVIC_ClearPendingIRQ(ADC_IRQn);
    HAL_NVIC_SetPriority(ADC_IRQn,4,0);HAL_NVIC_EnableIRQ(ADC_IRQn);
    last_sample=HAL_GetTick()-SENSOR_PERIOD_MS;
}
static uint8_t sample(uint32_t channel,uint16_t *value)
{
    ADC_ChannelConfTypeDef cfg={0};
    cfg.Channel=channel;
    cfg.Rank=1;
    cfg.SamplingTime=ADC_SAMPLETIME_480CYCLES;
    if (HAL_ADC_ConfigChannel(&adc,&cfg)!=HAL_OK) return 0;
    uint32_t sum=0;
    /* Discard first conversion after switching inputs. */
    for (unsigned i=0;i<=SENSOR_AVERAGES;i++) {
        if (HAL_ADC_Start(&adc)!=HAL_OK) return 0;
        if (HAL_ADC_PollForConversion(&adc,5)!=HAL_OK) {
            HAL_ADC_Stop(&adc);
            return 0;
        }
        uint16_t raw=(uint16_t)HAL_ADC_GetValue(&adc);
        HAL_ADC_Stop(&adc);
        if(i) sum+=raw;
    }
    *value=(uint16_t)((sum+SENSOR_AVERAGES/2)/SENSOR_AVERAGES);
    return 1;
}
uint8_t Sensors_Process(void)
{
    uint32_t now=HAL_GetTick();
    if((uint32_t)(now-last_sample)<SENSOR_PERIOD_MS && applied_low==SensorLimits_Get()->temp_low && applied_high==SensorLimits_Get()->temp_high) return 0;
    if(!ready && (uint32_t)(now-retry_at)>=2000U) Sensors_Init();
    const SensorLimits *limits=SensorLimits_Get();
    if(ready && (applied_low!=limits->temp_low || applied_high!=limits->temp_high)) {
        HAL_NVIC_DisableIRQ(ADC_IRQn);HAL_ADC_Stop(&adc);
        CLEAR_BIT(adc.Instance->CR2,ADC_CR2_CONT);
        __HAL_ADC_CLEAR_FLAG(&adc,ADC_FLAG_AWD);HAL_NVIC_ClearPendingIRQ(ADC_IRQn);
        alarm_active=alarm_pending=0;ready=watchdog_init();
        applied_low=limits->temp_low;applied_high=limits->temp_high;
        HAL_NVIC_EnableIRQ(ADC_IRQn);
    }
    last_sample=now;
    /* Pause background temperature conversion before sampling the light channel. */
    if(ready) {HAL_ADC_Stop(&adc);CLEAR_BIT(adc.Instance->CR2,ADC_CR2_CONT);}
    readings.light_valid=ready && sample(LIGHT_ADC_CHANNEL,&readings.light_raw);
    readings.temp_valid=ready && sample(TEMP_ADC_CHANNEL,&readings.temp_raw);
    uint8_t temp_read=readings.temp_valid,light_read=readings.light_valid;
    readings.sampled_at=HAL_GetTick();sampled=1;
    if(readings.light_valid) {
        unsigned level=((uint32_t)readings.light_raw*100U+2047U)/4095U;
        readings.light_percent=LIGHT_HIGH_IS_BRIGHT ? level : 100U-level;
    }
    if(readings.temp_valid) {
        unsigned raw=readings.temp_raw;
        if(raw<=5U || raw>=4090U) readings.temp_valid=0;
        else {
            float ratio=(float)raw/(4095.0f-raw);
            float r=NTC_FIXED_OHM*(NTC_TO_GROUND ? ratio : 1.0f/ratio);
            float c=1.0f/(1.0f/298.15f+logf(r/NTC_R25_OHM)/NTC_BETA_K)-273.15f;
            if(!isfinite(c) || c < -40.0f || c > 125.0f) readings.temp_valid=0;
            else readings.temp_tenths_c=(int16_t)lroundf(c*10.0f);
        }
    }
    if(ready) {
        float hyst=fminf(1.0f,(limits->temp_high-limits->temp_low)/4.0f);
        if(alarm_active && !alarm_pending && readings.temp_valid &&
           readings.temp_tenths_c >= (int)((limits->temp_low+hyst)*10) &&
           readings.temp_tenths_c <= (int)((limits->temp_high-hyst)*10)) {
            alarm_active=0;
            __HAL_ADC_CLEAR_FLAG(&adc,ADC_FLAG_AWD);
            __HAL_ADC_ENABLE_IT(&adc,ADC_IT_AWD);
        }
        /* Temperature is the last selected channel. Keep converting while the
         * main loop handles menus or blocking OLED transfers. Only AWD has IRQ. */
        ADC_ChannelConfTypeDef cfg={0};cfg.Channel=TEMP_ADC_CHANNEL;cfg.Rank=1;
        cfg.SamplingTime=ADC_SAMPLETIME_480CYCLES;
        if(HAL_ADC_ConfigChannel(&adc,&cfg)==HAL_OK) {
            SET_BIT(adc.Instance->CR2,ADC_CR2_CONT);
            if(HAL_ADC_Start(&adc)!=HAL_OK) {readings.temp_valid=0;temp_read=0;}
        } else {readings.temp_valid=0;temp_read=0;}
    }
    uint8_t fault=SensorFault_Update(&faults,ready,temp_read,readings.temp_raw,readings.temp_valid,light_read,readings.light_raw);
    if(fault&(SF_ADC|SF_TEMP_MASK)) readings.temp_valid=0;
    if(fault&(SF_ADC|SF_LIGHT_MASK)) readings.light_valid=0;
    return 1;
}
const SensorReadings *Sensors_Get(void) { return &readings; }
