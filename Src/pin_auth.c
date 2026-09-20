#include "pin_auth.h"
#include "app_policy.h"
#include <string.h>
void PIN_Clear(PinAuth *p) {memset(p->digits,0,sizeof(p->digits));p->count=0;}
void PIN_Back(PinAuth *p) {if(p->count) p->digits[--p->count]=0;}
uint8_t PIN_IsPaused(PinAuth *p,uint32_t now) {
    if(p->paused && (uint32_t)(now-p->pause_at)>=AUTH_PAUSE_MS) {p->paused=0;p->failures=0;}
    return p->paused;
}
Role PIN_Add(PinAuth *p,uint8_t digit,uint32_t now) {
    if(PIN_IsPaused(p,now)||digit>9) return ROLE_NONE;
    p->digits[p->count++]=(char)('0'+digit);
    if(p->count<4) return ROLE_NONE;
    Role r=strcmp(p->digits,PIN_ADMIN)==0 ? ROLE_ADMIN : (strcmp(p->digits,PIN_DISPENSER)==0 ? ROLE_DISPENSER : ROLE_NONE);
    PIN_Clear(p);
    if(r!=ROLE_NONE) p->failures=0;
    else if(++p->failures>=3) {p->paused=1;p->pause_at=now;}
    return r;
}
