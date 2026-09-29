#include "sensor_fault.h"
#include "app_policy.h"
static uint8_t next(uint8_t n) {return n<3 ? n+1:3;}
uint8_t SensorFault_Update(SensorFaultState *f,uint8_t ready,uint8_t tr,
    uint16_t raw,uint8_t tv,uint8_t lr,uint16_t light) {
    uint8_t temp=0,illum=0;
    if(!ready) {f->mask=SF_ADC;f->temp_good=f->light_good=0;return f->mask;}
    f->mask&=(uint8_t)~SF_ADC;
    if(!tr) temp=SF_TEMP_READ;
    else if(raw<=5 || raw>=4090) temp=SF_TEMP_RAIL;
    else if(!tv) temp=SF_TEMP_RANGE;
    if(!lr) {illum=SF_LIGHT_READ;f->light_rail=0;}
    else if(light<=1 || light>=4094) {
        f->light_rail=next(f->light_rail);
        if(f->light_rail>=3) illum=SF_LIGHT_RAIL;
    } else f->light_rail=0;
    if(temp) {f->mask=(f->mask&~SF_TEMP_MASK)|temp;f->temp_good=0;}
    else if((f->temp_good=next(f->temp_good))>=3) f->mask&=(uint8_t)~SF_TEMP_MASK;
    if(illum) {f->mask=(f->mask&~SF_LIGHT_MASK)|illum;f->light_good=0;}
    else if(!f->light_rail && (f->light_good=next(f->light_good))>=3) f->mask&=(uint8_t)~SF_LIGHT_MASK;
    return f->mask;
}
uint8_t SensorFault_WithAge(uint8_t mask,uint8_t sampled,uint32_t now,uint32_t at) {
    return mask|((!sampled || (uint32_t)(now-at)>=SENSOR_STALE_MS) ? SF_STALE:0);
}
