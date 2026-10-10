#include "app_constants.h"
#ifndef PIN_AUTH_H
#define PIN_AUTH_H
#include <stdint.h>

typedef enum
{
    ROLE_NONE,
    ROLE_DISPENSER,
    ROLE_ADMIN
} Role;

typedef struct
{
    char digits[PIN_BUFFER_SIZE];
    uint8_t count;
    uint8_t failures;
    uint8_t paused;
    uint32_t pause_at;
} PinAuth;

void PIN_Clear(PinAuth *p);
void PIN_Back(PinAuth *p);
Role PIN_Add(PinAuth *p, uint8_t digit, uint32_t now);
uint8_t PIN_IsPaused(PinAuth *p, uint32_t now);
Role PIN_AddRequired(PinAuth *p, uint8_t digit, uint32_t now, Role required);
void PIN_ResetConfig(void);
uint8_t PIN_SetCode(Role role, const char *code);
#endif
