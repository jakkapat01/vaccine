#ifndef APP_STATE_H
#define APP_STATE_H
#include "pin_auth.h"
#include "vial_model.h"
typedef enum {ST_BOOT,ST_PIN,ST_COOLDOWN,ST_MODE,ST_TRANSPORT,ST_VIAL,ST_REVIEW,ST_UNLOCKED,ST_OPEN,ST_CLOSE,ST_NOTICE,ST_HOME,ST_SETTINGS,ST_SET_TIME,ST_SET_PIN,ST_SETTINGS_MSG,ST_SET_RANGE} AppState;
typedef enum {MODE_TRANSPORT,MODE_DISPENSE,MODE_SERVICE} AppMode;
typedef struct {
    uint32_t now;
    float inside_c;
    uint8_t temp_valid,light_valid,light,sensor_fault;
} AppInput;
typedef struct {
    AppState state;AppMode mode;Role role;PinAuth pin;Vial vials[3];
    float outside_c,inside_c;
    uint32_t last_tick,last_activity,open_at,open_limit_s;
    uint8_t selected,choice,risk,lock_command,lid_open,overdue,temp_valid,light;
    uint8_t sensor_fault;
    uint8_t pending_action,health_unknown,hardware_ready,transport_guard;
    uint16_t edit_seconds;
    int16_t edit_low,edit_high;uint8_t range_light,range_high;
    uint8_t edit_count,edit_repeat;Role edit_role;char edit_pin[5],edit_check[5];
    const char *notice;
} App;
enum {KEY_CONFIRM=1,KEY_BACK=2,KEY_LEFT=4,KEY_RIGHT=8,KEY_OPEN=16};
void App_Init(App *a,uint32_t now);
void App_Tick(App *a,const AppInput *in);
void App_ServoOpened(App *a,uint32_t now);
void App_Key(App *a,uint8_t key,uint32_t now);
uint8_t App_AdminConsole(const App *a,uint32_t now);
const char *App_StateName(AppState state);
#endif
