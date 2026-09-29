#include "app_state.h"
#include "app_policy.h"
#include "risk_model.h"
#include "event_log.h"
#include "sensor_limits.h"
#include <string.h>
static void login(App *a) {memset(a->edit_pin,0,5);memset(a->edit_check,0,5);a->edit_count=a->edit_repeat=0;a->role=ROLE_NONE;PIN_Clear(&a->pin);a->state=ST_PIN;a->pending_action=0;a->choice=0;}
static void home(App *a) {login(a);a->state=a->transport_guard ? ST_TRANSPORT:ST_HOME;}
static void notice(App *a,const char *message) {a->notice=message;a->state=ST_NOTICE;}
void App_Init(App *a,uint32_t now) {
    memset(a,0,sizeof(*a));Vials_Init(a->vials);a->outside_c=35;a->last_tick=now;a->last_activity=now;a->state=ST_HOME;a->open_limit_s=60;
    /* lock_command: 0=no PWM yet, 1=locked command, 2=unlocked command. */
}
void App_Tick(App *a,const AppInput *in) {
    uint32_t dt=in->now-a->last_tick;
    /* Charge elapsed time to the preceding sample and lid state. */
    Vials_Update(a->vials,dt/1000.0f,a->inside_c,a->temp_valid,a->lid_open,a->light);
    if(!a->temp_valid && dt && a->state!=ST_BOOT) a->health_unknown=1;
    a->sensor_fault=in->sensor_fault;
    if(a->sensor_fault) a->health_unknown=1;
    a->last_tick=in->now;a->inside_c=in->inside_c;a->temp_valid=in->temp_valid;
    a->light=in->light_valid ? in->light : 0;
    a->risk=Risk_Calculate(a->inside_c,a->outside_c,&a->vials[a->selected],a->temp_valid);
    if(a->state==ST_COOLDOWN && !PIN_IsPaused(&a->pin,in->now)) home(a);
    if(a->role!=ROLE_NONE && a->lock_command==1 && (uint32_t)(in->now-a->last_activity)>=SESSION_IDLE_MS) home(a);
    /* TIM2 interrupt owns open-duration expiry; main consumes its flag. */
}
uint8_t App_AdminConsole(const App *a,uint32_t now) {
    return a->role==ROLE_ADMIN && a->lock_command==1 && (a->state==ST_MODE||a->state==ST_TRANSPORT) && (uint32_t)(now-a->last_activity)<SESSION_IDLE_MS;
}
static void handle_key(App *a,uint8_t key,uint32_t now) {
    if(a->state==ST_COOLDOWN) return;
    if(key==KEY_OPEN) {
        /* D2 requests entry at HOME; elsewhere it confirms the current step. */
        if(a->state==ST_CLOSE || a->state==ST_OPEN) key=KEY_CONFIRM;
        else if(a->state==ST_HOME || a->state==ST_TRANSPORT) {
            if(!a->hardware_ready) return;
            a->lock_command=1;login(a);a->last_activity=now;return;
        } else key=KEY_CONFIRM;
    }
    if(a->state>=ST_SETTINGS && (a->role!=ROLE_ADMIN || a->lock_command!=1 || a->transport_guard)) {home(a);return;}
    a->last_activity=now;
    if(key==KEY_LEFT || key==KEY_RIGHT) {
        unsigned count=0;
        if(a->state==ST_SET_RANGE) {
            int16_t *v=a->range_high ? &a->edit_high:&a->edit_low;
            int min=a->range_light ? 0:-40,max=a->range_light ? 100:125;
            if(key==KEY_RIGHT && *v<max) ++*v;
            if(key==KEY_LEFT && *v>min) --*v;
            return;
        }
        if(a->state==ST_SET_TIME) {
            if(key==KEY_RIGHT) a->edit_seconds=a->edit_seconds>295 ? 300:a->edit_seconds+5;
            else a->edit_seconds=a->edit_seconds<15 ? 10:a->edit_seconds-5;
            return;
        }
        if(a->state==ST_PIN || a->state==ST_SET_PIN) count=10;
        else if(a->state==ST_MODE) count=a->role==ROLE_ADMIN ? 4:3;
        else if(a->state==ST_VIAL) count=3;
        else if(a->state==ST_SETTINGS) count=5;
        if(count) a->choice=(uint8_t)((a->choice+(key==KEY_RIGHT ? 1U : count-1U))%count);
        return;
    }
    if(key==KEY_BACK) {
        switch(a->state) {
        case ST_PIN: if(a->pin.count) PIN_Back(&a->pin);else home(a);break;
        case ST_SETTINGS: a->state=ST_MODE;break;
        case ST_SET_RANGE: if(a->range_high) a->range_high=0;else a->state=ST_SETTINGS;break;
        case ST_SET_TIME: case ST_SETTINGS_MSG: a->state=ST_SETTINGS;break;
        case ST_SET_PIN:
            if(a->edit_count) {char *p=a->edit_repeat ? a->edit_check:a->edit_pin;p[--a->edit_count]=0;}
            else {memset(a->edit_pin,0,5);memset(a->edit_check,0,5);a->state=ST_SETTINGS;}
            break;
        case ST_MODE: home(a);break;
        case ST_TRANSPORT: break;
        case ST_VIAL: if(a->role==ROLE_ADMIN) a->state=ST_MODE;else home(a);break;
        case ST_NOTICE: a->state=a->role==ROLE_NONE ? ST_PIN : (a->role==ROLE_ADMIN ? ST_MODE:ST_VIAL);break;
        case ST_REVIEW: a->state=ST_VIAL;break;
        case ST_UNLOCKED: a->pending_action=0;a->state=ST_CLOSE;break;
        case ST_CLOSE: if(a->lid_open) {a->pending_action=0;a->state=ST_OPEN;}break;
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
        Role r=PIN_AddRequired(&a->pin,a->choice,now,a->transport_guard ? ROLE_ADMIN:ROLE_NONE);
        if(a->pin.paused) a->state=ST_COOLDOWN;
        else if(r!=ROLE_NONE) {if(a->transport_guard) EventLog_Add(now,"TRANSPORT END ADMIN");a->transport_guard=0;a->role=r;if(r==ROLE_DISPENSER) {a->mode=MODE_DISPENSE;a->state=ST_VIAL;}else a->state=ST_MODE;EventLog_Add(now,r==ROLE_ADMIN ? "LOGIN ADMIN":"LOGIN NURSE");}
        break;
    }
    case ST_SETTINGS:
        if(a->choice==0) {a->edit_seconds=(uint16_t)a->open_limit_s;a->state=ST_SET_TIME;}
        else if(a->choice>=3) {const SensorLimits *l=SensorLimits_Get();a->range_light=a->choice==4;a->range_high=0;a->notice=0;a->edit_low=a->range_light ? l->light_low:l->temp_low;a->edit_high=a->range_light ? l->light_high:l->temp_high;a->state=ST_SET_RANGE;}
        else {a->edit_role=a->choice==1 ? ROLE_DISPENSER:ROLE_ADMIN;memset(a->edit_pin,0,5);memset(a->edit_check,0,5);a->edit_count=a->edit_repeat=0;a->state=ST_SET_PIN;}
        break;
    case ST_SET_RANGE:
        if(!a->range_high) {a->range_high=1;break;}
        if(!SensorLimits_Set(a->range_light,a->edit_low,a->edit_high)) {a->notice="MIN MUST BE < MAX";break;}
        EventLog_Add(now,a->range_light ? "LIGHT RANGE UPDATED":"TEMP RANGE UPDATED");a->notice="SAVED (RAM ONLY)";a->state=ST_SETTINGS_MSG;break;
    case ST_SET_TIME:
        a->open_limit_s=a->edit_seconds;EventLog_Add(now,"OLED OPEN TIME UPDATED");a->notice="SAVED (RAM ONLY)";a->state=ST_SETTINGS_MSG;break;
    case ST_SET_PIN: {
        char *p=a->edit_repeat ? a->edit_check:a->edit_pin;
        p[a->edit_count++]=(char)('0'+a->choice);
        if(a->edit_count<4) break;
        if(!a->edit_repeat) {a->edit_repeat=1;a->edit_count=0;a->choice=0;break;}
        if(strcmp(a->edit_pin,a->edit_check)) a->notice="PIN MISMATCH";
        else if(!PIN_SetCode(a->edit_role,a->edit_pin)) a->notice="PIN ALREADY USED";
        else {a->notice="SAVED (RAM ONLY)";EventLog_Add(now,a->edit_role==ROLE_ADMIN ? "OLED ADMIN PIN CHANGED":"OLED NURSE PIN CHANGED");}
        memset(a->edit_pin,0,5);memset(a->edit_check,0,5);a->edit_count=0;a->state=ST_SETTINGS_MSG;break;
    }
    case ST_SETTINGS_MSG: a->state=ST_SETTINGS;break;
    case ST_MODE:
        if(a->role!=ROLE_ADMIN) {if(a->role==ROLE_DISPENSER) {a->mode=MODE_DISPENSE;a->state=ST_VIAL;}else home(a);break;}
        if(a->choice==3) {if(a->role==ROLE_ADMIN && a->lock_command==1 && !a->transport_guard) a->state=ST_SETTINGS;else notice(a,"ADMIN REQUIRED");break;}
        a->mode=(AppMode)a->choice;
        if(a->mode==MODE_TRANSPORT) {
            if(a->role!=ROLE_ADMIN) notice(a,"ADMIN REQUIRED");
            else {a->transport_guard=1;a->state=ST_TRANSPORT;EventLog_Add(now,"TRANSPORT START ADMIN");}
        }
        else if(a->role==ROLE_NONE) notice(a,"LOGIN REQUIRED");
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
            if(a->sensor_fault || !a->temp_valid) {notice(a,"SENSOR FAULT");break;}
            if(a->vials[a->selected].status!=VIAL_AVAILABLE) {notice(a,"VIAL NOT AVAILABLE");break;}
            if(a->risk>=DISPENSE_RISK_LIMIT) {notice(a,"RISK TOO HIGH");break;}
        } else if(a->mode!=MODE_SERVICE||a->role!=ROLE_ADMIN) {notice(a,"ACCESS DENIED");break;}
        a->lock_command=2;a->pending_action=0;a->overdue=0;a->notice=0;a->state=ST_UNLOCKED;
        break;
    case ST_UNLOCKED: break; /* Runtime advances only after a successful servo command. */
    case ST_OPEN:
        a->pending_action=1;
        if(a->pending_action && a->mode==MODE_DISPENSE && a->vials[a->selected].status!=VIAL_AVAILABLE) {
            a->notice="QUARANTINED NO TAKE";a->pending_action=0;
        }
        a->state=ST_CLOSE;
        break;
    case ST_CLOSE:
        {
        uint8_t rejected=0;
        if(a->pending_action) {
            if(a->mode==MODE_SERVICE && a->role==ROLE_ADMIN) {Vial_Refill(&a->vials[a->selected]);EventLog_Add(now,"REPLACE VIAL CONFIRMED");}
            else if(a->mode==MODE_DISPENSE) rejected=!Vial_Return(&a->vials[a->selected]);
        }
        a->lid_open=0;a->lock_command=1;a->overdue=0;home(a);
        if(rejected) notice(a,"QUAR KEEP VIAL");
        break;
        }
    case ST_NOTICE: a->state=a->role==ROLE_NONE ? ST_PIN : (a->role==ROLE_ADMIN ? ST_MODE:ST_VIAL);break;
    default: break;
    }
}
void App_ServoOpened(App *a,uint32_t now) {
    if(a->state==ST_UNLOCKED && a->lock_command==2) {a->lid_open=1;a->open_at=now;a->state=ST_OPEN;}
}
void App_Key(App *a,uint8_t key,uint32_t now) {
    AppState before=a->state;
    handle_key(a,key,now);
    if(a->state!=before) a->choice=(a->state==ST_VIAL ? a->selected : 0);
}
const char *App_StateName(AppState s) {
    static const char *names[]={"BOOT","PIN","COOLDOWN","MODE","TRANSPORT","VIAL","REVIEW","UNLOCKED","OPEN","CLOSE","NOTICE","HOME","SETTINGS","SET_TIME","SET_PIN","SETTINGS_MSG","SET_RANGE"};
    return (unsigned)s<sizeof(names)/sizeof(names[0]) ? names[s] : "UNKNOWN";
}
