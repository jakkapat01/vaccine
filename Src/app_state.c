#include "app_state.h"
#include "app_policy.h"
#include "risk_model.h"
#include <string.h>
static void login(App *a) {a->role=ROLE_NONE;PIN_Clear(&a->pin);a->state=ST_PIN;a->pending_action=0;a->choice=0;}
static void notice(App *a,const char *message) {a->notice=message;a->state=ST_NOTICE;}
void App_Init(App *a,uint32_t now) {
    memset(a,0,sizeof(*a));Vials_Init(a->vials);a->outside_c=35;a->last_tick=now;a->last_activity=now;a->state=ST_BOOT;
    /* lock_command: 0=no PWM yet, 1=locked command, 2=unlocked command. */
}
void App_Tick(App *a,const AppInput *in) {
    uint32_t dt=in->now-a->last_tick;
    /* Charge elapsed time to the preceding sample and lid state. */
    Vials_Update(a->vials,dt/1000.0f,a->inside_c,a->temp_valid,a->lid_open,a->light);
    if(!a->temp_valid && dt && a->state!=ST_BOOT) a->health_unknown=1;
    a->last_tick=in->now;a->inside_c=in->inside_c;a->temp_valid=in->temp_valid;
    a->light=in->light_valid ? in->light : 0;
    a->risk=Risk_Calculate(a->inside_c,a->outside_c,&a->vials[a->selected],a->temp_valid);
    if(a->state==ST_COOLDOWN && !PIN_IsPaused(&a->pin,in->now)) login(a);
    if(a->role!=ROLE_NONE && a->lock_command==1 && (uint32_t)(in->now-a->last_activity)>=SESSION_IDLE_MS) login(a);
    if(a->lid_open && (uint32_t)(in->now-a->open_at)>=(a->mode==MODE_SERVICE ? SERVICE_OPEN_MS : DISPENSE_OPEN_MS)) a->overdue=1;
}
uint8_t App_AdminConsole(const App *a,uint32_t now) {
    return a->role==ROLE_ADMIN && a->lock_command==1 && (a->state==ST_MODE||a->state==ST_TRANSPORT) && (uint32_t)(now-a->last_activity)<SESSION_IDLE_MS;
}
static void handle_key(App *a,uint8_t key,uint32_t now) {
    if(a->state==ST_COOLDOWN) return;
    a->last_activity=now;
    if(key==KEY_LEFT || key==KEY_RIGHT) {
        unsigned count=0;
        if(a->state==ST_PIN) count=10;
        else if(a->state==ST_MODE || a->state==ST_VIAL) count=3;
        else if(a->state==ST_RESULT) count=2;
        if(count) a->choice=(uint8_t)((a->choice+(key==KEY_RIGHT ? 1U : count-1U))%count);
        return;
    }
    if(key==KEY_BACK) {
        switch(a->state) {
        case ST_PIN: PIN_Back(&a->pin);break;
        case ST_MODE: login(a);break;
        case ST_TRANSPORT: case ST_VIAL: a->state=ST_MODE;break;
        case ST_NOTICE: a->state=a->role==ROLE_NONE ? ST_PIN : ST_MODE;break;
        case ST_REVIEW: a->state=ST_VIAL;break;
        case ST_UNLOCKED: a->pending_action=0;a->state=ST_CLOSE;break;
        case ST_RESULT: a->state=ST_OPEN;break;
        case ST_CLOSE: if(a->lid_open) a->state=ST_RESULT;break;
        default: break;
        }
        return;
    }
    if(key!=KEY_CONFIRM) return;
    switch(a->state) {
    case ST_BOOT:
        if(a->hardware_ready) {a->lock_command=1;login(a);}
        break;
    case ST_PIN: {
        Role r=PIN_Add(&a->pin,a->choice,now);
        if(a->pin.paused) a->state=ST_COOLDOWN;
        else if(r!=ROLE_NONE) {a->role=r;a->state=ST_MODE;}
        break;
    }
    case ST_MODE:
        a->mode=(AppMode)a->choice;
        if(a->mode==MODE_TRANSPORT) a->state=ST_TRANSPORT;
        else if(a->mode==MODE_SERVICE && a->role!=ROLE_ADMIN) notice(a,"ADMIN REQUIRED");
        else a->state=ST_VIAL;
        break;
    case ST_VIAL:
        a->selected=a->choice;
        if(a->mode==MODE_DISPENSE && a->vials[a->selected].status!=VIAL_AVAILABLE) notice(a,"VIAL NOT AVAILABLE");
        else a->state=ST_REVIEW;
        break;
    case ST_REVIEW:
        a->risk=Risk_Calculate(a->inside_c,a->outside_c,&a->vials[a->selected],a->temp_valid);
        if(!a->hardware_ready) {notice(a,"HARDWARE FAULT");break;}
        if(a->mode==MODE_DISPENSE) {
            if(a->role==ROLE_NONE) {notice(a,"LOGIN REQUIRED");break;}
            if(!a->temp_valid) {notice(a,"TEMP SENSOR FAULT");break;}
            if(a->vials[a->selected].status!=VIAL_AVAILABLE) {notice(a,"VIAL NOT AVAILABLE");break;}
            if(a->risk>=DISPENSE_RISK_LIMIT) {notice(a,"RISK TOO HIGH");break;}
        } else if(a->mode!=MODE_SERVICE||a->role!=ROLE_ADMIN) {notice(a,"ACCESS DENIED");break;}
        a->lock_command=2;a->pending_action=0;a->overdue=0;a->notice=0;a->state=ST_UNLOCKED;
        break;
    case ST_UNLOCKED: a->lid_open=1;a->open_at=now;a->state=ST_OPEN;break;
    case ST_OPEN: a->state=ST_RESULT;break;
    case ST_RESULT:
        a->pending_action=a->choice;
        if(a->pending_action && a->mode==MODE_DISPENSE && a->vials[a->selected].status!=VIAL_AVAILABLE) {
            a->notice="QUARANTINED NO TAKE";a->pending_action=0;
        }
        a->state=ST_CLOSE;
        break;
    case ST_CLOSE:
        {
        uint8_t rejected=0;
        if(a->pending_action) {
            if(a->mode==MODE_SERVICE && a->role==ROLE_ADMIN) Vial_Refill(&a->vials[a->selected]);
            else if(a->mode==MODE_DISPENSE) rejected=!Vial_Dispense(&a->vials[a->selected]);
        }
        a->lid_open=0;a->lock_command=1;a->overdue=0;login(a);
        if(rejected) notice(a,"QUAR KEEP VIAL");
        break;
        }
    case ST_NOTICE: a->state=a->role==ROLE_NONE ? ST_PIN : ST_MODE;break;
    default: break;
    }
}
void App_Key(App *a,uint8_t key,uint32_t now) {
    AppState before=a->state;
    handle_key(a,key,now);
    if(a->state!=before) a->choice=(a->state==ST_VIAL ? a->selected : 0);
}
const char *App_StateName(AppState s) {
    static const char *names[]={"BOOT","PIN","COOLDOWN","MODE","TRANSPORT","VIAL","REVIEW","UNLOCKED","OPEN","RESULT","CLOSE","NOTICE"};
    return (unsigned)s<sizeof(names)/sizeof(names[0]) ? names[s] : "UNKNOWN";
}
