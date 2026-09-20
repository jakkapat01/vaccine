#include "console_commands.h"
#include "app_policy.h"
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
        snprintf(out,size,"STATUS | SET OUTSIDE_TEMP -40..60 | SET VIAL 1..3 SENSITIVITY 0.1..5 | LOGOUT\r\nSET requires board ADMIN login at MODE/TRANSPORT. No SET HEALTH.\r\n");return;
    }
    if(n==1 && !strcmp(tokens[0],"STATUS")) {
        int t=(int)(a->inside_c*10),outside=(int)(a->outside_c*10);
        snprintf(out,size,"STATE %s ROLE %u IN_REAL %s%d.%d C VALID %u OUT_SIM %s%d.%d C LIGHT %u RISK %u LID_SIM %s LOCK_CMD %u GAP %u\r\nV1 H%u S%u V2 H%u S%u V3 H%u S%u (S0 AVAILABLE S1 QUARANTINED S2 DISPENSED)\r\n",
        App_StateName(a->state),(unsigned)a->role,t<0 ? "-":"",abs(t)/10,abs(t)%10,(unsigned)a->temp_valid,outside<0 ? "-":"",abs(outside)/10,abs(outside)%10,(unsigned)a->light,(unsigned)a->risk,a->lid_open ? "OPEN":"CLOSED_OR_UNKNOWN",(unsigned)a->lock_command,(unsigned)a->health_unknown,
        (unsigned)a->vials[0].health,(unsigned)a->vials[0].status,(unsigned)a->vials[1].health,(unsigned)a->vials[1].status,(unsigned)a->vials[2].health,(unsigned)a->vials[2].status);return;
    }
    if(n==1 && !strcmp(tokens[0],"LOGOUT")) {
        if(a->lock_command!=1) {snprintf(out,size,"ERROR CLOSE_LID_FIRST\r\n");return;}
        a->role=ROLE_NONE;PIN_Clear(&a->pin);
        if(a->state!=ST_COOLDOWN) a->state=ST_PIN;
        snprintf(out,size,"OK LOGOUT\r\n");return;
    }
    if(strcmp(tokens[0],"SET")) {snprintf(out,size,"ERROR COMMAND\r\n");return;}
    if(!App_AdminConsole(a,now)) {snprintf(out,size,"ERROR ADMIN_LOGIN_AT_MODE_REQUIRED\r\n");return;}
    float value;
    if(n==3 && !strcmp(tokens[1],"OUTSIDE_TEMP")) {
        if(!number(tokens[2],&value)||value<OUTSIDE_MIN_C||value>OUTSIDE_MAX_C) {snprintf(out,size,"ERROR RANGE_OR_FORMAT\r\n");return;}
        a->outside_c=value;
    } else if(n==5 && !strcmp(tokens[1],"VIAL") && strlen(tokens[2])==1 && tokens[2][0]>='1' && tokens[2][0]<='3' && !strcmp(tokens[3],"SENSITIVITY")) {
        if(!number(tokens[4],&value)||value<SENSITIVITY_MIN||value>SENSITIVITY_MAX) {snprintf(out,size,"ERROR RANGE_OR_FORMAT\r\n");return;}
        a->vials[tokens[2][0]-'1'].sensitivity=value;
    } else {snprintf(out,size,"ERROR COMMAND_OR_ARGUMENTS\r\n");return;}
    a->last_activity=now;snprintf(out,size,"OK\r\n");
}
