#include "app_constants.h"
#include "pin_auth.h"
#include "app_policy.h"
#include <string.h>
static char admin_code[PIN_BUFFER_SIZE] = PIN_ADMIN;
static char nurse_code[PIN_BUFFER_SIZE] = PIN_DISPENSER;

void PIN_ResetConfig(void)
{
    strcpy(admin_code, PIN_ADMIN);
    strcpy(nurse_code, PIN_DISPENSER);
}

uint8_t PIN_SetCode(Role role, const char *code)
{
    if (((role != ROLE_ADMIN) && (role != ROLE_DISPENSER)) || (strlen(code) != PIN_DIGITS))
    {
        return 0U;
    }
    else
    {
        /* No action. */
    }
    for (unsigned i = 0U; (i < PIN_DIGITS); i++)
    {
        if ((code[i] < '0') || (code[i] > '9'))
        {
            return 0U;
        }
        else
        {
            /* No action. */
        }
    }
    if (role == ROLE_ADMIN)
    {
        if (strcmp(code, nurse_code) == 0)
        {
            return 0U;
        }
        else
        {
            /* No action. */
        }
    }
    else
    {
        if (strcmp(code, admin_code) == 0)
        {
            return 0U;
        }
        else
        {
            /* No action. */
        }
    }
    if (role == ROLE_ADMIN)
    {
        strcpy(admin_code, code);
    }
    else
    {
        strcpy(nurse_code, code);
    }
    return 1U;
}

void PIN_Clear(PinAuth *p)
{
    memset(p->digits, 0, sizeof(p->digits));
    p->count = 0U;
}

void PIN_Back(PinAuth *p)
{
    if (p->count != 0U)
    {
        p->digits[--p->count] = 0;
    }
    else
    {
        /* No action. */
    }
}

uint8_t PIN_IsPaused(PinAuth *p, uint32_t now)
{
    if ((p->paused != 0U) && ((uint32_t)(now - p->pause_at) >= AUTH_PAUSE_MS))
    {
        p->paused = 0U;
        p->failures = 0U;
    }
    else
    {
        /* No action. */
    }
    return p->paused;
}

Role PIN_AddRequired(PinAuth *p, uint8_t digit, uint32_t now, Role required)
{
    if ((PIN_IsPaused(p, now) != 0U) || (digit > PIN_MAX_DIGIT))
    {
        return ROLE_NONE;
    }
    else
    {
        /* No action. */
    }
    p->digits[p->count++] = (char)('0' + digit);
    if (p->count < PIN_DIGITS)
    {
        return ROLE_NONE;
    }
    else
    {
        /* No action. */
    }
    Role r;
    if (strcmp(p->digits, admin_code) == 0)
    {
        r = ROLE_ADMIN;
    }
    else
    {
        if (strcmp(p->digits, nurse_code) == 0)
        {
            r = ROLE_DISPENSER;
        }
        else
        {
            r = ROLE_NONE;
        }
    }
    if ((required != ROLE_NONE) && (r != required))
    {
        r = ROLE_NONE;
    }
    else
    {
        /* No action. */
    }
    PIN_Clear(p);
    if (r != ROLE_NONE)
    {
        p->failures = 0U;
    }
    else if (++p->failures >= PIN_FAILURE_LIMIT)
    {
        p->paused = 1U;
        p->pause_at = now;
    }
    else
    {
        /* No action. */
    }
    return r;
}

Role PIN_Add(PinAuth *p, uint8_t digit, uint32_t now)
{
    return PIN_AddRequired(p, digit, now, ROLE_NONE);
}
