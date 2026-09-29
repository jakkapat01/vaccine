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
App vaccine_app; /* Debugger view; no PC setter for Health. */
static uint8_t oled_ok,servo_ok,alarm_popup,alarm_was_active;
static uint32_t screen_at,retry_at;
static AppState last_state;
static uint8_t light_alarm,light_pending;
static int16_t last_light_low=0,last_light_high=100;
static uint8_t timer_open,last_fault;
void AppRuntime_Init(void) {
    WarningLEDs_Init();EventLog_Init();PIN_ResetConfig();LidTimer_Init();timer_open=0;
    Buttons_Init();Sensors_Init();Sensors_Process();
    App_Init(&vaccine_app,HAL_GetTick());
    UARTConsole_Init();oled_ok=OLED_Init();servo_ok=Servo_Ready();
    last_state=ST_BOOT;screen_at=HAL_GetTick()-250U;
}
void AppRuntime_Process(void) {
    Sensors_Process();const SensorReadings *s=Sensors_Get();uint32_t now=HAL_GetTick();
    AppInput in={.now=now,.inside_c=s->temp_tenths_c/10.0f,.temp_valid=s->temp_valid && (uint32_t)(now-s->sampled_at)<SENSOR_STALE_MS,
        .light_valid=s->light_valid && (uint32_t)(now-s->sampled_at)<SENSOR_STALE_MS,.light=s->light_percent,.sensor_fault=Sensors_GetFaults(now)};
    vaccine_app.hardware_ready=oled_ok && servo_ok;
    App_Tick(&vaccine_app,&in);
    UARTConsole_Process(&vaccine_app);
    if(Sensors_TakeTempAlarm() && !in.sensor_fault) {
        char alarm[100];if(alarm_popup==2) light_pending=1;alarm_popup=1;screen_at=now-250U;
        snprintf(alarm,sizeof(alarm),"ALARM TEMP ADC_WATCHDOG IRQ_COUNT %lu LIMIT %d..%dC\r\n",(unsigned long)Sensors_TempAlarmCount(),SensorLimits_Get()->temp_low,SensorLimits_Get()->temp_high);
        UARTConsole_Log(alarm);EventLog_Add(now,"TEMP ADC WATCHDOG ALARM");
        while(Buttons_GetEvent()) {} /* Old confirms must not acknowledge a new alarm. */
    }
    const SensorLimits *limits=SensorLimits_Get();
    if(last_light_low!=limits->light_low || last_light_high!=limits->light_high) {light_alarm=light_pending=0;last_light_low=limits->light_low;last_light_high=limits->light_high;}
    int light_hyst=(limits->light_high-limits->light_low)/4;if(light_hyst>2) light_hyst=2;
    if(in.light_valid) {
        if(!light_alarm && (s->light_percent<limits->light_low || s->light_percent>limits->light_high)) {
            light_alarm=light_pending=1;UARTConsole_Log("ALARM LIGHT OUT OF RANGE\r\n");EventLog_Add(now,"LIGHT OUT OF RANGE");
        } else if(light_alarm && s->light_percent>=limits->light_low+light_hyst && s->light_percent<=limits->light_high-light_hyst) {
            light_alarm=0;UARTConsole_Log("ALARM LIGHT RECOVERED\r\n");
        }
    }
    if(light_pending && !alarm_popup && !in.sensor_fault) {light_pending=0;alarm_popup=2;screen_at=now-250U;while(Buttons_GetEvent()) {}}
    uint8_t active=Sensors_TempAlarmActive();
    if(alarm_was_active && !active) UARTConsole_Log("ALARM TEMP RECOVERED REARMED\r\n");
    alarm_was_active=active;
    uint8_t fresh_fault=in.sensor_fault && in.sensor_fault!=last_fault;
    if(fresh_fault) {
        char fault_log[110];alarm_popup=3;screen_at=now-250U;
        snprintf(fault_log,sizeof(fault_log),"SENSOR FAULT MASK=%u TEMP_RAW=%u LIGHT_RAW=%u\r\n",in.sensor_fault,s->temp_raw,s->light_raw);
        UARTConsole_Log(fault_log);EventLog_Add(now,fault_log);
        while(Buttons_GetEvent()) {}
    } else if(last_fault && !in.sensor_fault) {
        UARTConsole_Log("SENSOR RECOVERED\r\n");EventLog_Add(now,"SENSOR RECOVERED");screen_at=now-250U;
        if(alarm_popup==3) alarm_popup=0;
        if(Sensors_TempAlarmActive()) alarm_popup=1;
    }
    last_fault=in.sensor_fault;
    uint8_t request=Buttons_TakeOpenRequest();
    if(request && !alarm_popup && !fresh_fault) App_Key(&vaccine_app,KEY_OPEN,HAL_GetTick());
    uint8_t key=Buttons_GetEvent();
    if(alarm_popup) {
        if(key==KEY_BACK) {alarm_popup=0;screen_at=now-250U;UARTConsole_Log("ALARM ACK\r\n");}
    } else if(key) App_Key(&vaccine_app,key,HAL_GetTick());
    if(servo_ok) {
        if(!Servo_Apply(vaccine_app.lock_command)) {servo_ok=0;vaccine_app.hardware_ready=0;UARTConsole_Log("ERROR SERVO COMMAND FAILED\r\n");}
        else App_ServoOpened(&vaccine_app,HAL_GetTick());
    }
    if(vaccine_app.lid_open && !timer_open) {
        LidTimer_Start(vaccine_app.open_limit_s);timer_open=1;
        EventLog_Add(now,"SERVO OPEN COMMAND TIMER START");
    }
    if(LidTimer_TakeExpired()) {
        vaccine_app.overdue=1;screen_at=now-250U;
        EventLog_Add(now,"TIMEOUT TIM2 CLOSE LID");UARTConsole_Log("ALARM TIM2 TIMEOUT CLOSE LID\r\n");
    }
    if(!vaccine_app.lid_open && timer_open) {
        LidTimer_Stop();timer_open=0;EventLog_Add(now,"LID CLOSED CONFIRMED");
    }
    WarningLEDs_Process(vaccine_app.lid_open && vaccine_app.overdue,now);
    if(vaccine_app.state!=last_state) {
        char log[80];snprintf(log,sizeof(log),"EVENT %s LID_SIM %u LOCK_CMD %u\r\n",App_StateName(vaccine_app.state),(unsigned)vaccine_app.lid_open,(unsigned)vaccine_app.lock_command);
        UARTConsole_Log(log);EventLog_Add(now,log);last_state=vaccine_app.state;
    }
    if(!oled_ok && (uint32_t)(now-retry_at)>=2000U) {retry_at=now;oled_ok=OLED_Init();}
    if(oled_ok && (uint32_t)(now-screen_at)>=250U) {screen_at=now;oled_ok=alarm_popup==3 ? UI_SensorFault(in.sensor_fault):(alarm_popup==1 ? UI_TempAlarm(s->temp_tenths_c,vaccine_app.temp_valid):(alarm_popup==2 ? UI_LightAlarm(s->light_percent):UI_App(&vaccine_app,now)));}
}
