#include "vial_model.h"
#include "app_policy.h"
void Vials_Init(Vial v[3]) {
    for(unsigned i=0;i<3;i++) {v[i].health=100;v[i].sensitivity=1.0f+0.5f*i;v[i].status=VIAL_AVAILABLE;}
}
void Vials_Update(Vial v[3],float seconds,float t,uint8_t valid,uint8_t open,uint8_t light) {
    if(seconds<=0) return;
    float excursion=0;
    if(valid) {
        if(t<MODEL_TEMP_LOW_C) excursion=MODEL_TEMP_LOW_C-t;
        if(t>MODEL_TEMP_HIGH_C) excursion=t-MODEL_TEMP_HIGH_C;
    }
    /* Missing temperature is reported as unknown, not silently treated as safe.
       Known open-lid exposure still accumulates. No invented temperature. */
    float rate=excursion*HEALTH_TEMP_RATE+(open ? HEALTH_OPEN_RATE*(1.0f+light/100.0f) : 0);
    for(unsigned i=0;i<3;i++) if(v[i].status!=VIAL_DISPENSED) {
        v[i].health-=rate*seconds*v[i].sensitivity;
        if(v[i].health<=0) {v[i].health=0;v[i].status=VIAL_QUARANTINED;}
    }
}
uint8_t Vial_Dispense(Vial *v) {
    if(v->status!=VIAL_AVAILABLE || v->health<=0) return 0;
    v->status=VIAL_DISPENSED;return 1;
}
void Vial_Refill(Vial *v) {v->health=100;v->status=VIAL_AVAILABLE;}
