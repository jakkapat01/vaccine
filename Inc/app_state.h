#ifndef APP_STATE_H
#define APP_STATE_H
#include "pin_auth.h"
#include "vial_model.h"
typedef enum {ST_BOOT,ST_PIN,ST_COOLDOWN,ST_MODE,ST_TRANSPORT,ST_VIAL,ST_REVIEW,ST_UNLOCKED,ST_OPEN,ST_RESULT,ST_CLOSE,ST_NOTICE} AppState;
typedef enum {MODE_TRANSPORT,MODE_DISPENSE,MODE_SERVICE} AppMode;
typedef struct {
    uint32_t now;
    float inside_c;
    uint8_t temp_valid,light_valid,light;
} AppInput;
typedef struct {
    AppState state;AppMode mode;Role role;PinAuth pin;Vial vials[3];
    float outside_c,inside_c;
    uint32_t last_tick,last_activity,open_at;
    uint8_t selected,choice,risk,lock_command,lid_open,overdue,temp_valid,light;
    uint8_t pending_action,health_unknown,hardware_ready;
    const char *notice;
} App;
enum {KEY_CONFIRM=1,KEY_BACK=2,KEY_LEFT=4,KEY_RIGHT=8};
void App_Init(App *a,uint32_t now);
void App_Tick(App *a,const AppInput *in);
void App_Key(App *a,uint8_t key,uint32_t now);
uint8_t App_AdminConsole(const App *a,uint32_t now);
const char *App_StateName(AppState state);
#endif
