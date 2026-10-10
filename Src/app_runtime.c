#include "app_constants.h"
#define POPUP_TEMPERATURE 1U
#define POPUP_LIGHT 2U
#define POPUP_SENSOR_FAULT 3U
#include "main.h"
#include "warning_leds.h"
#include "lid_timer.h"
#include "event_log.h"
#include "app_runtime.h"
#include "app_state.h"
#include "app_policy.h"
#include "buttons.h"
#include "sensors.h"
#include "sensor_limits.h"
#include "servo_lock.h"
#include "ui_screen.h"
#include "oled_display.h"
#include "uart_console.h"
#include <stdio.h>
#define TEMP_ALARM_LOG_SIZE 100U
#define SENSOR_FAULT_LOG_SIZE 110U
#define STATE_LOG_SIZE 80U
App vaccine_app; /* Debugger view; no PC setter for Health. */
static uint8_t oled_ok;
static uint8_t servo_ok;
static uint8_t alarm_popup;
static uint8_t alarm_was_active;
static uint32_t screen_at;
static uint32_t retry_at;
static AppState last_state;
static uint8_t light_alarm;
static uint8_t light_pending;
static int16_t last_light_low = 0;
static int16_t last_light_high = LIGHT_PERCENT_MAX;
static uint8_t timer_open;
static uint8_t last_fault;

void AppRuntime_Init(void)
{
    WarningLEDs_Init();
    EventLog_Init();
    PIN_ResetConfig();
    LidTimer_Init();
    timer_open = 0U;
    Buttons_Init();
    Sensors_Init();
    Sensors_Process();
    App_Init(&vaccine_app, HAL_GetTick());
    UARTConsole_Init();
    oled_ok = OLED_Init();
    servo_ok = Servo_Ready();
    last_state = ST_BOOT;
    screen_at = (HAL_GetTick() - DISPLAY_PERIOD_MS);
}

void AppRuntime_Process(void)
{
    Sensors_Process();
    const SensorReadings *s = Sensors_Get();
    uint32_t now = HAL_GetTick();
    AppInput in = {.now = now,
                   .inside_c = (s->temp_tenths_c / 10.0f),
                   .temp_valid = ((s->temp_valid != 0U) &&
                                  ((uint32_t)(now - s->sampled_at) < SENSOR_STALE_MS)),
                   .light_valid = ((s->light_valid != 0U) &&
                                   ((uint32_t)(now - s->sampled_at) < SENSOR_STALE_MS)),
                   .light = s->light_percent,
                   .sensor_fault = Sensors_GetFaults(now)};
    vaccine_app.hardware_ready = ((oled_ok != 0U) && (servo_ok != 0U));
    App_Tick(&vaccine_app, &in);
    UARTConsole_Process(&vaccine_app);
    if ((Sensors_TakeTempAlarm() != 0U) && (in.sensor_fault == 0U))
    {
        char alarm[TEMP_ALARM_LOG_SIZE];
        if (alarm_popup == POPUP_LIGHT)
        {
            light_pending = 1U;
        }
        else
        {
            /* No action. */
        }
        alarm_popup = POPUP_TEMPERATURE;
        screen_at = (now - DISPLAY_PERIOD_MS);
        snprintf(alarm, sizeof(alarm), "ALARM TEMP ADC_WATCHDOG IRQ_COUNT %lu LIMIT %d..%dC\r\n",
                 (unsigned long)Sensors_TempAlarmCount(), SensorLimits_Get()->temp_low,
                 SensorLimits_Get()->temp_high);
        UARTConsole_Log(alarm);
        EventLog_Add(now, "TEMP ADC WATCHDOG ALARM");
        while (Buttons_GetEvent() != 0U)
        {
        } /* Old confirms must not acknowledge a new alarm. */
    }
    else
    {
        /* No action. */
    }
    const SensorLimits *limits = SensorLimits_Get();
    if ((last_light_low != limits->light_low) || (last_light_high != limits->light_high))
    {
        light_pending = 0U;
        light_alarm = 0U;
        last_light_low = limits->light_low;
        last_light_high = limits->light_high;
    }
    else
    {
        /* No action. */
    }
    /* The ADC completion ISR owns threshold detection and hysteresis.
     * Main only consumes events and performs OLED/UART/log operations. */
    if ((in.light_valid != 0U) && (Sensors_TakeLightAlarm() != 0U))
    {
        light_pending = 1U;
        UARTConsole_Log("ALARM LIGHT OUT OF RANGE ADC_EOC IRQ\r\n");
        EventLog_Add(now, "LIGHT ADC EOC ALARM");
    }
    else
    {
        /* Preserve the pending ISR event while SensorFault masks the sample. */
    }
    uint8_t light_active = Sensors_LightAlarmActive();
    if ((light_alarm != 0U) && (light_active == 0U))
    {
        UARTConsole_Log("ALARM LIGHT RECOVERED\r\n");
    }
    else
    {
        /* No action. */
    }
    light_alarm = light_active;
    if (((light_pending != 0U) && (alarm_popup == 0U)) && (in.sensor_fault == 0U))
    {
        light_pending = 0U;
        alarm_popup = POPUP_LIGHT;
        screen_at = (now - DISPLAY_PERIOD_MS);
        while (Buttons_GetEvent() != 0U)
        {
        }
    }
    else
    {
        /* No action. */
    }
    uint8_t active = Sensors_TempAlarmActive();
    if ((alarm_was_active != 0U) && (active == 0U))
    {
        UARTConsole_Log("ALARM TEMP RECOVERED REARMED\r\n");
    }
    else
    {
        /* No action. */
    }
    alarm_was_active = active;
    uint8_t fresh_fault = ((in.sensor_fault != 0U) && (in.sensor_fault != last_fault));
    if (fresh_fault != 0U)
    {
        char fault_log[SENSOR_FAULT_LOG_SIZE];
        alarm_popup = POPUP_SENSOR_FAULT;
        screen_at = (now - DISPLAY_PERIOD_MS);
        snprintf(fault_log, sizeof(fault_log), "SENSOR FAULT MASK=%u TEMP_RAW=%u LIGHT_RAW=%u\r\n",
                 in.sensor_fault, s->temp_raw, s->light_raw);
        UARTConsole_Log(fault_log);
        EventLog_Add(now, fault_log);
        while (Buttons_GetEvent() != 0U)
        {
        }
    }
    else if ((last_fault != 0U) && (in.sensor_fault == 0U))
    {
        UARTConsole_Log("SENSOR RECOVERED\r\n");
        EventLog_Add(now, "SENSOR RECOVERED");
        screen_at = (now - DISPLAY_PERIOD_MS);
        if (alarm_popup == POPUP_SENSOR_FAULT)
        {
            alarm_popup = 0U;
        }
        else
        {
            /* No action. */
        }
        if (Sensors_TempAlarmActive() != 0U)
        {
            alarm_popup = POPUP_TEMPERATURE;
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
    last_fault = in.sensor_fault;
    uint8_t request = Buttons_TakeOpenRequest();
    if (((request != 0U) && (alarm_popup == 0U)) && (fresh_fault == 0U))
    {
        App_Key(&vaccine_app, KEY_OPEN, HAL_GetTick());
    }
    else
    {
        /* No action. */
    }
    uint8_t key = Buttons_GetEvent();
    if (alarm_popup != 0U)
    {
        if (key == KEY_BACK)
        {
            alarm_popup = 0U;
            screen_at = (now - DISPLAY_PERIOD_MS);
            UARTConsole_Log("ALARM ACK\r\n");
        }
        else
        {
            /* No action. */
        }
    }
    else if (key != 0U)
    {
        App_Key(&vaccine_app, key, HAL_GetTick());
    }
    else
    {
        /* No action. */
    }
    if (servo_ok != 0U)
    {
        if (Servo_Apply(vaccine_app.lock_command) == 0U)
        {
            servo_ok = 0U;
            vaccine_app.hardware_ready = 0U;
            UARTConsole_Log("ERROR SERVO COMMAND FAILED\r\n");
        }
        else
        {
            App_ServoOpened(&vaccine_app, HAL_GetTick());
        }
    }
    else
    {
        /* No action. */
    }
    if ((vaccine_app.lid_open != 0U) && (timer_open == 0U))
    {
        LidTimer_Start(vaccine_app.open_limit_s);
        timer_open = 1U;
        EventLog_Add(now, "SERVO OPEN COMMAND TIMER START");
    }
    else
    {
        /* No action. */
    }
    if (LidTimer_TakeExpired() != 0U)
    {
        vaccine_app.overdue = 1U;
        screen_at = (now - DISPLAY_PERIOD_MS);
        EventLog_Add(now, "TIMEOUT TIM2 CLOSE LID");
        UARTConsole_Log("ALARM TIM2 TIMEOUT CLOSE LID\r\n");
    }
    else
    {
        /* No action. */
    }
    if ((vaccine_app.lid_open == 0U) && (timer_open != 0U))
    {
        LidTimer_Stop();
        timer_open = 0U;
        EventLog_Add(now, "LID CLOSED CONFIRMED");
    }
    else
    {
        /* No action. */
    }
    WarningLEDs_Process(((vaccine_app.lid_open != 0U) && (vaccine_app.overdue != 0U)), now);
    if (vaccine_app.state != last_state)
    {
        char log[STATE_LOG_SIZE];
        snprintf(log, sizeof(log), "EVENT %s LID_SIM %u LOCK_CMD %u\r\n",
                 App_StateName(vaccine_app.state), (unsigned)vaccine_app.lid_open,
                 (unsigned)vaccine_app.lock_command);
        UARTConsole_Log(log);
        EventLog_Add(now, log);
        last_state = vaccine_app.state;
    }
    else
    {
        /* No action. */
    }
    if ((oled_ok == 0U) && ((uint32_t)(now - retry_at) >= HARDWARE_RETRY_MS))
    {
        retry_at = now;
        oled_ok = OLED_Init();
    }
    else
    {
        /* No action. */
    }
    if ((oled_ok != 0U) && ((uint32_t)(now - screen_at) >= DISPLAY_PERIOD_MS))
    {
        screen_at = now;
        if (alarm_popup == POPUP_SENSOR_FAULT)
        {
            oled_ok = UI_SensorFault(in.sensor_fault);
        }
        else
        {
            if (alarm_popup == POPUP_TEMPERATURE)
            {
                oled_ok = UI_TempAlarm(s->temp_tenths_c, vaccine_app.temp_valid);
            }
            else
            {
                if (alarm_popup == POPUP_LIGHT)
                {
                    oled_ok = UI_LightAlarm(s->light_percent);
                }
                else
                {
                    oled_ok = UI_App(&vaccine_app, now);
                }
            }
        }
    }
    else
    {
        /* No action. */
    }
}
