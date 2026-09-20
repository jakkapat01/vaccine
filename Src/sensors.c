#include "main.h"
#include "sensors.h"
#include "sensor_config.h"
#include <math.h>
static ADC_HandleTypeDef adc;
static SensorReadings readings;
static uint32_t last_sample;
static uint8_t ready;

void Sensors_Init(void)
{
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
    ready=(HAL_ADC_Init(&adc)==HAL_OK);
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
    if((uint32_t)(now-last_sample)<SENSOR_PERIOD_MS) return 0;
    last_sample=now;
    readings.light_valid=ready && sample(LIGHT_ADC_CHANNEL,&readings.light_raw);
    readings.temp_valid=ready && sample(TEMP_ADC_CHANNEL,&readings.temp_raw);
    readings.sampled_at=HAL_GetTick();
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
    return 1;
}
const SensorReadings *Sensors_Get(void) { return &readings; }
