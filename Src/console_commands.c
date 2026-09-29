#include "console_commands.h"
#include "app_policy.h"
#include "event_log.h"
#include "sensor_limits.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <errno.h>
static uint8_t number(const char *s,float *value) {
    if(!*s) return 0;
    char *end;errno=0;float v=strtof(s,&end);
    if(errno || end==s || *end || !isfinite(v)) return 0;
    *value=v;return 1;
}
void Console_Command(App *a,const char *line,uint32_t now,char *out,size_t size) {
    char text[96],*tokens[6];unsigned n=0;
    if(strlen(line)>=sizeof(text)) {snprintf(out,size,"ERROR LINE_TOO_LONG\r\n");return;}
    strcpy(text,line);
    char *token=strtok(text," \t");
    while(token && n<6) {tokens[n++]=token;token=strtok(NULL," \t");}
    if(!n) {snprintf(out,size,"ERROR EMPTY\r\n");return;}
    if(n==1 && !strcmp(tokens[0],"HELP")) {
        snprintf(out,size,"STATUS | IRQ | CONFIG | LOG 0..3 | LOGOUT | SET OPEN_SECONDS 10..300 | SET PIN NURSE/ADMIN dddd | SET OUTSIDE_TEMP -40..60 | SET VIAL 1..3 SENSITIVITY 0.1..5\r\nSET requires board ADMIN login at MODE/TRANSPORT. No SET HEALTH.\r\n");return;
    }
    if(n==1 && !strcmp(tokens[0],"STATUS")) {
        int t=(int)(a->inside_c*10),outside=(int)(a->outside_c*10);
        snprintf(out,size,"STATE %s ROLE %u IN_REAL %s%d.%d C VALID %u OUT_SIM %s%d.%d C LIGHT %u RISK %u LID_SIM %s LOCK_CMD %u GAP %u SENSOR_FAULT %u\r\nV1 H%u S%u V2 H%u S%u V3 H%u S%u (S0 AVAILABLE S1 QUARANTINED)\r\n",
        App_StateName(a->state),(unsigned)a->role,t<0 ? "-":"",abs(t)/10,abs(t)%10,(unsigned)a->temp_valid,outside<0 ? "-":"",abs(outside)/10,abs(outside)%10,(unsigned)a->light,(unsigned)a->risk,a->lid_open ? "OPEN":"CLOSED_OR_UNKNOWN",(unsigned)a->lock_command,(unsigned)a->health_unknown,(unsigned)a->sensor_fault,
        (unsigned)a->vials[0].health,(unsigned)a->vials[0].status,(unsigned)a->vials[1].health,(unsigned)a->vials[1].status,(unsigned)a->vials[2].health,(unsigned)a->vials[2].status);return;
    }
    if(n==1 && !strcmp(tokens[0],"LOGOUT")) {
        if(a->lock_command!=1) {snprintf(out,size,"ERROR CLOSE_LID_FIRST\r\n");return;}
        a->role=ROLE_NONE;PIN_Clear(&a->pin);
        if(a->state!=ST_COOLDOWN) a->state=a->transport_guard ? ST_TRANSPORT:ST_HOME;
        snprintf(out,size,"OK LOGOUT\r\n");return;
    }
    if(n==1 && !strcmp(tokens[0],"CONFIG")) {
        if(!App_AdminConsole(a,now)) {snprintf(out,size,"ERROR ADMIN REQUIRED\r\n");return;}
        snprintf(out,size,"OPEN_SECONDS %lu TRANSPORT %u TEMP_RANGE %d..%dC LIGHT_RANGE %d..%d%% SETTINGS RAM ONLY\r\n",(unsigned long)a->open_limit_s,(unsigned)a->transport_guard,SensorLimits_Get()->temp_low,SensorLimits_Get()->temp_high,SensorLimits_Get()->light_low,SensorLimits_Get()->light_high);return;
    }
    if(!strcmp(tokens[0],"LOG")) {
        if(!App_AdminConsole(a,now)) {snprintf(out,size,"ERROR ADMIN REQUIRED\r\n");return;}
        if(n!=2||strlen(tokens[1])!=1||tokens[1][0]<'0'||tokens[1][0]>'3') {snprintf(out,size,"ERROR LOG 0..3\r\n");return;}
        EventLog_Read(tokens[1][0]-'0',out,size);return;
    }
    if(strcmp(tokens[0],"SET")) {snprintf(out,size,"ERROR COMMAND\r\n");return;}
    if(!App_AdminConsole(a,now)) {snprintf(out,size,"ERROR ADMIN_LOGIN_AT_MODE_REQUIRED\r\n");return;}
    float value;
    if(n==3 && !strcmp(tokens[1],"OPEN_SECONDS")) {
        if(!number(tokens[2],&value)||value<10||value>300||value!=(unsigned)value) {snprintf(out,size,"ERROR RANGE 10..300 INTEGER\r\n");return;}
        a->open_limit_s=(uint32_t)value;
    } else if(n==4 && !strcmp(tokens[1],"PIN")) {
        Role role=!strcmp(tokens[2],"ADMIN") ? ROLE_ADMIN:(!strcmp(tokens[2],"NURSE") ? ROLE_DISPENSER:ROLE_NONE);
        if(!PIN_SetCode(role,tokens[3])) {snprintf(out,size,"ERROR PIN 4 DIGITS AND UNIQUE ROLE\r\n");return;}
        EventLog_Add(now,role==ROLE_ADMIN ? "CONFIG ADMIN PIN CHANGED":"CONFIG NURSE PIN CHANGED");
        a->last_activity=now;snprintf(out,size,"OK PIN CHANGED RAM ONLY\r\n");return;
    } else
    if(n==3 && !strcmp(tokens[1],"OUTSIDE_TEMP")) {
        if(!number(tokens[2],&value)||value<OUTSIDE_MIN_C||value>OUTSIDE_MAX_C) {snprintf(out,size,"ERROR RANGE_OR_FORMAT\r\n");return;}
        a->outside_c=value;
    } else if(n==5 && !strcmp(tokens[1],"VIAL") && strlen(tokens[2])==1 && tokens[2][0]>='1' && tokens[2][0]<='3' && !strcmp(tokens[3],"SENSITIVITY")) {
        if(!number(tokens[4],&value)||value<SENSITIVITY_MIN||value>SENSITIVITY_MAX) {snprintf(out,size,"ERROR RANGE_OR_FORMAT\r\n");return;}
        a->vials[tokens[2][0]-'1'].sensitivity=value;
    } else {snprintf(out,size,"ERROR COMMAND_OR_ARGUMENTS\r\n");return;}
    EventLog_Add(now,"CONFIG UPDATED");a->last_activity=now;snprintf(out,size,"OK RAM ONLY\r\n");
}
