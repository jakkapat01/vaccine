#include "app_constants.h"
#ifndef VIAL_MODEL_H
#define VIAL_MODEL_H
#include <stdint.h>

typedef enum
{
    VIAL_AVAILABLE,
    VIAL_QUARANTINED
} VialStatus;

typedef struct
{
    float health;
    float sensitivity;
    VialStatus status;
} Vial;

void Vials_Init(Vial v[VIAL_COUNT]);
void Vials_Update(Vial v[VIAL_COUNT], float seconds, float inside_c, uint8_t valid, uint8_t open,
                  uint8_t light);
uint8_t Vial_Return(Vial *v);
void Vial_Refill(Vial *v);
#endif
