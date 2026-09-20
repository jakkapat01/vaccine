#ifndef APP_POLICY_H
#define APP_POLICY_H
/* Classroom demonstration parameters, NOT vaccine quality criteria. */
#define PIN_DISPENSER "1234"
#define PIN_ADMIN "2468"
#define AUTH_PAUSE_MS 30000U
#define SESSION_IDLE_MS 120000U
#define DISPENSE_OPEN_MS 60000U
#define SERVICE_OPEN_MS 120000U
#define SENSOR_STALE_MS 2000U
#define MODEL_TEMP_LOW_C 2.0f
#define MODEL_TEMP_HIGH_C 8.0f
#define MODEL_TEMP_RISK_WEIGHT 2.0f
#define MODEL_OUTSIDE_RISK_WEIGHT 1.0f
#define MODEL_HEALTH_RISK_WEIGHT 0.4f
#define DISPENSE_RISK_LIMIT 70U
#define HEALTH_TEMP_RATE 0.002f
#define HEALTH_OPEN_RATE 0.05f
#define OUTSIDE_MIN_C (-40.0f)
#define OUTSIDE_MAX_C 60.0f
#define SENSITIVITY_MIN 0.1f
#define SENSITIVITY_MAX 5.0f
#endif
