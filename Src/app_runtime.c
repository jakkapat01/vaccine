#include "main.h"
#include "app_runtime.h"
#include "app_state.h"
#include "app_policy.h"
#include "buttons.h"
#include "sensors.h"
#include "servo_lock.h"
#include "ui_screen.h"
#include "oled_display.h"
#include "uart_console.h"
#include <stdio.h>
App vaccine_app; /* Debugger view; no PC setter for Health. */
static uint8_t oled_ok,servo_ok;
static uint32_t screen_at,retry_at;
static AppState last_state;
void AppRuntime_Init(void) {
    Buttons_Init();Sensors_Init();Sensors_Process();
    App_Init(&vaccine_app,HAL_GetTick());
    UARTConsole_Init();oled_ok=OLED_Init();servo_ok=Servo_Ready();
    last_state=ST_BOOT;screen_at=HAL_GetTick()-250U;
}
void AppRuntime_Process(void) {
    Sensors_Process();const SensorReadings *s=Sensors_Get();uint32_t now=HAL_GetTick();
    AppInput in={.now=now,.inside_c=s->temp_tenths_c/10.0f,.temp_valid=s->temp_valid && (uint32_t)(now-s->sampled_at)<SENSOR_STALE_MS,
        .light_valid=s->light_valid,.light=s->light_percent};
    vaccine_app.hardware_ready=oled_ok && servo_ok;
    App_Tick(&vaccine_app,&in);
    UARTConsole_Process(&vaccine_app);
    uint8_t key=Buttons_GetEvent();if(key) App_Key(&vaccine_app,key,HAL_GetTick());
    if(servo_ok && !Servo_Apply(vaccine_app.lock_command)) {servo_ok=0;vaccine_app.hardware_ready=0;UARTConsole_Log("ERROR SERVO COMMAND FAILED\r\n");}
    if(vaccine_app.state!=last_state) {
        char log[80];snprintf(log,sizeof(log),"EVENT %s LID_SIM %u LOCK_CMD %u\r\n",App_StateName(vaccine_app.state),(unsigned)vaccine_app.lid_open,(unsigned)vaccine_app.lock_command);
        UARTConsole_Log(log);last_state=vaccine_app.state;
    }
    if(!oled_ok && (uint32_t)(now-retry_at)>=2000U) {retry_at=now;oled_ok=OLED_Init();}
    if(oled_ok && (uint32_t)(now-screen_at)>=250U) {screen_at=now;oled_ok=UI_App(&vaccine_app,now);}
}
