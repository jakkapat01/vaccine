#ifndef VIAL_MODEL_H
#define VIAL_MODEL_H
#include <stdint.h>
typedef enum {VIAL_AVAILABLE,VIAL_QUARANTINED,VIAL_DISPENSED} VialStatus;
typedef struct { float health,sensitivity; VialStatus status; } Vial;
void Vials_Init(Vial v[3]);
void Vials_Update(Vial v[3],float seconds,float inside_c,uint8_t valid,uint8_t open,uint8_t light);
uint8_t Vial_Dispense(Vial *v);
void Vial_Refill(Vial *v);
#endif
