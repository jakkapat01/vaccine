#include "pin_auth.h"
#include "app_policy.h"
#include <string.h>
static char admin_code[5]=PIN_ADMIN,nurse_code[5]=PIN_DISPENSER;
void PIN_ResetConfig(void) {strcpy(admin_code,PIN_ADMIN);strcpy(nurse_code,PIN_DISPENSER);}
uint8_t PIN_SetCode(Role role,const char *code) {
    if((role!=ROLE_ADMIN && role!=ROLE_DISPENSER)||strlen(code)!=4) return 0;
    for(unsigned i=0;i<4;i++) if(code[i]<'0'||code[i]>'9') return 0;
    if(!strcmp(code,role==ROLE_ADMIN ? nurse_code:admin_code)) return 0;
    strcpy(role==ROLE_ADMIN ? admin_code:nurse_code,code);return 1;
}
void PIN_Clear(PinAuth *p) {memset(p->digits,0,sizeof(p->digits));p->count=0;}
void PIN_Back(PinAuth *p) {if(p->count) p->digits[--p->count]=0;}
uint8_t PIN_IsPaused(PinAuth *p,uint32_t now) {
    if(p->paused && (uint32_t)(now-p->pause_at)>=AUTH_PAUSE_MS) {p->paused=0;p->failures=0;}
    return p->paused;
}
Role PIN_AddRequired(PinAuth *p,uint8_t digit,uint32_t now,Role required) {
    if(PIN_IsPaused(p,now)||digit>9) return ROLE_NONE;
    p->digits[p->count++]=(char)('0'+digit);
    if(p->count<4) return ROLE_NONE;
    Role r=strcmp(p->digits,admin_code)==0 ? ROLE_ADMIN : (strcmp(p->digits,nurse_code)==0 ? ROLE_DISPENSER : ROLE_NONE);
    if(required!=ROLE_NONE && r!=required) r=ROLE_NONE;
    PIN_Clear(p);
    if(r!=ROLE_NONE) p->failures=0;
    else if(++p->failures>=3) {p->paused=1;p->pause_at=now;}
    return r;
}

Role PIN_Add(PinAuth *p,uint8_t digit,uint32_t now) {return PIN_AddRequired(p,digit,now,ROLE_NONE);}
