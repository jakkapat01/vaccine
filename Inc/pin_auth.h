#ifndef PIN_AUTH_H
#define PIN_AUTH_H
#include <stdint.h>
typedef enum {ROLE_NONE,ROLE_DISPENSER,ROLE_ADMIN} Role;
typedef struct {char digits[5];uint8_t count,failures,paused;uint32_t pause_at;} PinAuth;
void PIN_Clear(PinAuth *p);
void PIN_Back(PinAuth *p);
Role PIN_Add(PinAuth *p,uint8_t digit,uint32_t now);
uint8_t PIN_IsPaused(PinAuth *p,uint32_t now);
#endif
