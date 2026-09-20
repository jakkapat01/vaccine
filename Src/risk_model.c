#include "risk_model.h"
#include "app_policy.h"
uint8_t Risk_Calculate(float t,float outside,const Vial *v,uint8_t valid) {
    if(!valid) return 100;
    float excursion=t<MODEL_TEMP_LOW_C ? MODEL_TEMP_LOW_C-t : (t>MODEL_TEMP_HIGH_C ? t-MODEL_TEMP_HIGH_C : 0);
    float warm=outside>t ? outside-t : 0;
    float score=excursion*MODEL_TEMP_RISK_WEIGHT+warm*MODEL_OUTSIDE_RISK_WEIGHT+(100-v->health)*MODEL_HEALTH_RISK_WEIGHT;
    if(score>100) score=100;
    if(score<0) score=0;
    return (uint8_t)(score+0.5f);
}
