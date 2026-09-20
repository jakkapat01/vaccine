#ifndef SENSOR_CONFIG_H
#define SENSOR_CONFIG_H
/* Shield labels interpreted from the supplied photo; confirm wiring.
 * These are EXTERNAL sensors, not the MCU junction-temperature sensor. */
#define LIGHT_ADC_CHANNEL ADC_CHANNEL_1
#define TEMP_ADC_CHANNEL ADC_CHANNEL_0
#define SENSOR_GPIO_PINS (GPIO_PIN_0 | GPIO_PIN_1)
/* Provisional NTC divider parameters from Training_Lab4.2.
 * Confirm against the shield schematic before treating C as calibrated. */
#define NTC_FIXED_OHM 10000.0f
#define NTC_R25_OHM 10000.0f
#define NTC_BETA_K 3950.0f
#define NTC_TO_GROUND 1
#define LIGHT_HIGH_IS_BRIGHT 0
#define SENSOR_PERIOD_MS 500U
#define SENSOR_AVERAGES 8U
#endif
