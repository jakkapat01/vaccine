#include "ui_screen.h"
#include "oled_display.h"
#include "app_policy.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

static void center(unsigned y,const char *s,unsigned scale) {
    unsigned width=(unsigned)strlen(s)*6U*scale;
    OLED_Text((uint8_t)(width<120U ? (128U-width)/2U : 4U),(uint8_t)y,s,(uint8_t)scale);
}
static void header(const char *s) {OLED_Text(4,0,s,1);OLED_Rect(4,10,120,1,1);}
static void footer(const char *s) {OLED_Rect(4,53,120,1,1);center(56,s,1);}
static void navigation(void) {footer("4< 5> 2OK 3BACK");}
static unsigned health_value(const Vial *v) {
    if(!isfinite(v->health)||v->health<=0) return 0;
    if(v->health>=100) return 100;
    unsigned h=(unsigned)(v->health+0.5f);
    return h ? h : 1U;
}
static void vial_row(const Vial *v,unsigned index,unsigned y,uint8_t selected) {
    char text[8];unsigned health=health_value(v);
    if(selected) OLED_Text(4,(uint8_t)(y+1),">",1);
    snprintf(text,sizeof(text),"V%u",index+1U);OLED_Text(12,(uint8_t)(y+1),text,1);
    OLED_Rect(29,y,66,10,0);
    if(v->status==VIAL_DISPENSED) OLED_Text(53,(uint8_t)(y+1),"OUT",1);
    else if(v->status==VIAL_QUARANTINED) OLED_Text(50,(uint8_t)(y+1),"QUAR",1);
    else if(!isfinite(v->health)) OLED_Text(53,(uint8_t)(y+1),"ERR",1);
    else if(health) {
        unsigned fill=(62U*health+50U)/100U;
        OLED_Rect(31,y+2,fill ? fill:1U,6,1);
    }
    snprintf(text,sizeof(text),"%u%%",health);
    OLED_Text((uint8_t)(124U-strlen(text)*6U),(uint8_t)(y+1),text,1);
}
static void vials(const App *a,uint8_t selection) {
    for(unsigned i=0;i<3;i++) vial_row(&a->vials[i],i,14U+i*13U,selection && a->choice==i);
}
static void temperature(char *out,size_t size,float t) {
    int d=(int)lroundf(t*10.0f);
    snprintf(out,size,"%s%d.%d",d<0 ? "-":"",abs(d)/10,abs(d)%10);
}
uint8_t UI_App(const App *a,uint32_t now) {
    char text[32],in[10],outside[10];
    OLED_Clear();
    switch(a->state) {
    case ST_BOOT:
        header("NEW SESSION");center(19,"CLOSE LID",2);
        center(39,a->hardware_ready ? "LID SIM":"HARDWARE ERROR",1);
        footer(a->hardware_ready ? "D2 CLOSED":"WAIT");break;
    case ST_PIN: {
        snprintf(text,sizeof(text),a->pin.failures ? "PIN  FAIL %u OF 3":"PIN",(unsigned)a->pin.failures);
        header(text);char mask[5]="----";
        for(unsigned i=0;i<a->pin.count;i++) mask[i]='*';
        center(14,mask,2);
        snprintf(text,sizeof(text),"%u",(unsigned)a->choice);center(34,text,2);
        OLED_Text(37,38,"<",1);OLED_Text(85,38,">",1);navigation();break;
    }
    case ST_COOLDOWN: {
        uint32_t elapsed=now-a->pin.pause_at;
        unsigned remaining=elapsed>=AUTH_PAUSE_MS ? 0U:(AUTH_PAUSE_MS-elapsed+999U)/1000U;
        header("TRY AGAIN IN");snprintf(text,sizeof(text),"%u S",remaining);center(23,text,2);footer("MONITORING ACTIVE");break;
    }
    case ST_MODE: {
        header(a->role==ROLE_ADMIN ? "ADMIN":"DISPENSER");
        const char *m[]={"TRANSPORT","DISPENSE","SERVICE"};
        for(unsigned i=0;i<3;i++) {
            unsigned y=15U+i*13U;
            if(a->choice==i) {OLED_Rect(4,y-2,120,12,0);OLED_Text(8,(uint8_t)y,">",1);}
            OLED_Text(21,(uint8_t)y,m[i],1);
        }
        navigation();break;
    }
    case ST_TRANSPORT:
        temperature(in,sizeof(in),a->inside_c);
        snprintf(text,sizeof(text),"IN %sC",a->temp_valid ? in:"--");header(text);
        OLED_Text(100,0,a->temp_valid ? "LOCK":"ERR",1);
        vials(a,0);footer(a->health_unknown ? "GAP  D3 BACK":"D3 BACK");break;
    case ST_VIAL:
        header(a->mode==MODE_SERVICE ? "SERVICE VIAL":"PICK VIAL");vials(a,1);navigation();break;
    case ST_REVIEW:
        snprintf(text,sizeof(text),a->mode==MODE_SERVICE ? "SERVICE V%u":"DISPENSE V%u",a->selected+1U);header(text);
        temperature(in,sizeof(in),a->inside_c);temperature(outside,sizeof(outside),a->outside_c);
        snprintf(text,sizeof(text),"IN%s SIM%s",a->temp_valid ? in:"ERR",outside);center(13,text,1);
        snprintf(text,sizeof(text),"RISK %u",(unsigned)a->risk);center(24,text,2);
        vial_row(&a->vials[a->selected],a->selected,41,0);
        footer(a->mode==MODE_SERVICE ? "2OVERRIDE 3BACK":"2OPEN 3BACK");break;
    case ST_UNLOCKED:
        header("LID SIM");center(21,"OPEN LID",2);footer("2OPENED 3CANCEL");break;
    case ST_OPEN:
        if(a->overdue) header("TIMEOUT - CLOSE LID");
        else {snprintf(text,sizeof(text),"OPEN SIM %luS",(unsigned long)((now-a->open_at)/1000U));header(text);}
        vials(a,0);footer("D2 FINISH");break;
    case ST_RESULT:
        snprintf(text,sizeof(text),a->overdue ? "TIMEOUT V%u":"V%u RESULT",a->selected+1U);header(text);
        center(24,a->choice ? (a->mode==MODE_SERVICE ? "NEW VIAL":"DISPENSED"):"NO CHANGE",2);
        navigation();break;
    case ST_CLOSE:
        snprintf(text,sizeof(text),"V%u %s",a->selected+1U,a->pending_action ? (a->mode==MODE_SERVICE ? "NEW VIAL":"REMOVED"):"NO CHANGE");header(text);
        center(19,"CLOSE LID",2);
        center(39,a->notice ? "QUAR - KEEP VIAL":(a->overdue ? "TIMEOUT":"LID SIM"),1);
        footer("2CLOSED 3EDIT");break;
    case ST_NOTICE:
        header("NO ACCESS");center(24,a->notice ? a->notice:"ERROR",1);footer("D2 BACK");break;
    }
    return OLED_Update();
}
