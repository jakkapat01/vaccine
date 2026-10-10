#include "app_constants.h"
#define COMMAND_VALUE_TOKENS 3U
#define COMMAND_PIN_TOKENS 4U
#define COMMAND_VIAL_TOKENS 5U
#define COMMAND_LOG_TOKENS 2U
#include <stdbool.h>
#include "console_commands.h"
#include "app_policy.h"
#include "event_log.h"
#include "sensor_limits.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <errno.h>

/* Select display/log text without changing either string. */
static const char *select_text(bool condition, const char *when_true, const char *when_false)
{
    if (condition)
    {
        return when_true;
    }
    else
    {
        return when_false;
    }
}

static uint8_t number(const char *s, float *value)
{
    if (*s == 0)
    {
        return 0U;
    }
    else
    {
        /* No action. */
    }
    char *end;
    errno = 0;
    float v = strtof(s, &end);
    if ((((errno != 0) || (end == s)) || (*end != 0)) || (isfinite(v) == 0))
    {
        return 0U;
    }
    else
    {
        /* No action. */
    }
    *value = v;
    return 1U;
}

void Console_Command(App *a, const char *line, uint32_t now, char *out, size_t size)
{
    char text[SERIAL_LINE_SIZE];
    char *tokens[SERIAL_TOKEN_COUNT];
    unsigned n = 0U;
    if (strlen(line) >= sizeof(text))
    {
        snprintf(out, size, "ERROR LINE_TOO_LONG\r\n");
        return;
    }
    else
    {
        /* No action. */
    }
    strcpy(text, line);
    char *token = strtok(text, " \t");
    while ((token != 0) && (n < SERIAL_TOKEN_COUNT))
    {
        tokens[n++] = token;
        token = strtok(NULL, " \t");
    }
    if (n == 0U)
    {
        snprintf(out, size, "ERROR EMPTY\r\n");
        return;
    }
    else
    {
        /* No action. */
    }
    if ((n == 1U) && (strcmp(tokens[0U], "HELP") == 0))
    {
        snprintf(out, size,
                 "STATUS | IRQ | CONFIG | LOG 0..3 | LOGOUT | SET OPEN_SECONDS 10..300 | SET PIN "
                 "NURSE/ADMIN dddd | SET OUTSIDE_TEMP -40..60 | SET VIAL 1..3 SENSITIVITY "
                 "0.1..5\r\nSET requires board ADMIN login at MODE/TRANSPORT. No SET HEALTH.\r\n");
        return;
    }
    else
    {
        /* No action. */
    }
    if ((n == 1U) && (strcmp(tokens[0U], "STATUS") == 0))
    {
        int t = (int)(a->inside_c * DECIMAL_BASE);
        int outside = (int)(a->outside_c * DECIMAL_BASE);
        snprintf(out, size,
                 "STATE %s ROLE %u IN_REAL %s%d.%d C VALID %u OUT_SIM %s%d.%d C LIGHT %u RISK %u "
                 "LID_SIM %s LOCK_CMD %u GAP %u SENSOR_FAULT %u\r\nV1 H%u S%u V2 H%u S%u V3 H%u "
                 "S%u (S0 AVAILABLE S1 QUARANTINED)\r\n",
                 App_StateName(a->state), (unsigned)a->role, select_text((t < 0), "-", ""),
                 (abs(t) / DECIMAL_BASE), (abs(t) % DECIMAL_BASE), (unsigned)a->temp_valid,
                 select_text((outside < 0), "-", ""), (abs(outside) / DECIMAL_BASE),
                 (abs(outside) % DECIMAL_BASE), (unsigned)a->light, (unsigned)a->risk,
                 select_text((a->lid_open != 0U), "OPEN", "CLOSED_OR_UNKNOWN"),
                 (unsigned)a->lock_command, (unsigned)a->health_unknown, (unsigned)a->sensor_fault,
                 (unsigned)a->vials[0U].health, (unsigned)a->vials[0U].status,
                 (unsigned)a->vials[1U].health, (unsigned)a->vials[1U].status,
                 (unsigned)a->vials[2U].health, (unsigned)a->vials[2U].status);
        return;
    }
    else
    {
        /* No action. */
    }
    if ((n == 1U) && (strcmp(tokens[0U], "LOGOUT") == 0))
    {
        if (a->lock_command != LOCK_CLOSED_COMMAND)
        {
            snprintf(out, size, "ERROR CLOSE_LID_FIRST\r\n");
            return;
        }
        else
        {
            /* No action. */
        }
        a->role = ROLE_NONE;
        PIN_Clear(&a->pin);
        if (a->state != ST_COOLDOWN)
        {
            if (a->transport_guard != 0U)
            {
                a->state = ST_TRANSPORT;
            }
            else
            {
                a->state = ST_HOME;
            }
        }
        else
        {
            /* No action. */
        }
        snprintf(out, size, "OK LOGOUT\r\n");
        return;
    }
    else
    {
        /* No action. */
    }
    if ((n == 1U) && (strcmp(tokens[0U], "CONFIG") == 0))
    {
        if (App_AdminConsole(a, now) == 0U)
        {
            snprintf(out, size, "ERROR ADMIN REQUIRED\r\n");
            return;
        }
        else
        {
            /* No action. */
        }
        snprintf(out, size,
                 "OPEN_SECONDS %lu TRANSPORT %u TEMP_RANGE %d..%dC LIGHT_RANGE %d..%d%% SETTINGS "
                 "RAM ONLY\r\n",
                 (unsigned long)a->open_limit_s, (unsigned)a->transport_guard,
                 SensorLimits_Get()->temp_low, SensorLimits_Get()->temp_high,
                 SensorLimits_Get()->light_low, SensorLimits_Get()->light_high);
        return;
    }
    else
    {
        /* No action. */
    }
    if (strcmp(tokens[0U], "LOG") == 0)
    {
        if (App_AdminConsole(a, now) == 0U)
        {
            snprintf(out, size, "ERROR ADMIN REQUIRED\r\n");
            return;
        }
        else
        {
            /* No action. */
        }
        if ((((n != COMMAND_LOG_TOKENS) || (strlen(tokens[1U]) != 1U)) || (tokens[1U][0U] < '0')) ||
            (tokens[1U][0U] > '3'))
        {
            snprintf(out, size, "ERROR LOG 0..3\r\n");
            return;
        }
        else
        {
            /* No action. */
        }
        EventLog_Read((tokens[1U][0U] - '0'), out, size);
        return;
    }
    else
    {
        /* No action. */
    }
    if (strcmp(tokens[0U], "SET") != 0)
    {
        snprintf(out, size, "ERROR COMMAND\r\n");
        return;
    }
    else
    {
        /* No action. */
    }
    if (App_AdminConsole(a, now) == 0U)
    {
        snprintf(out, size, "ERROR ADMIN_LOGIN_AT_MODE_REQUIRED\r\n");
        return;
    }
    else
    {
        /* No action. */
    }
    float value;
    if ((n == COMMAND_VALUE_TOKENS) && (strcmp(tokens[1U], "OPEN_SECONDS") == 0))
    {
        if ((((number(tokens[2U], &value) == 0U) || (value < OPEN_TIME_MIN_S)) ||
             (value > OPEN_TIME_MAX_S)) ||
            (value > (float)(unsigned)value))
        {
            snprintf(out, size, "ERROR RANGE 10..300 INTEGER\r\n");
            return;
        }
        else
        {
            /* No action. */
        }
        a->open_limit_s = (uint32_t)value;
    }
    else if ((n == COMMAND_PIN_TOKENS) && (strcmp(tokens[1U], "PIN") == 0))
    {
        Role role;
        if (strcmp(tokens[2U], "ADMIN") == 0)
        {
            role = ROLE_ADMIN;
        }
        else
        {
            if (strcmp(tokens[2U], "NURSE") == 0)
            {
                role = ROLE_DISPENSER;
            }
            else
            {
                role = ROLE_NONE;
            }
        }
        if (PIN_SetCode(role, tokens[3U]) == 0U)
        {
            snprintf(out, size, "ERROR PIN 4 DIGITS AND UNIQUE ROLE\r\n");
            return;
        }
        else
        {
            /* No action. */
        }
        EventLog_Add(now, select_text((role == ROLE_ADMIN), "CONFIG ADMIN PIN CHANGED",
                                      "CONFIG NURSE PIN CHANGED"));
        a->last_activity = now;
        snprintf(out, size, "OK PIN CHANGED RAM ONLY\r\n");
        return;
    }
    else if ((n == COMMAND_VALUE_TOKENS) && (strcmp(tokens[1U], "OUTSIDE_TEMP") == 0))
    {
        if (((number(tokens[2U], &value) == 0U) || (value < OUTSIDE_MIN_C)) ||
            (value > OUTSIDE_MAX_C))
        {
            snprintf(out, size, "ERROR RANGE_OR_FORMAT\r\n");
            return;
        }
        else
        {
            /* No action. */
        }
        a->outside_c = value;
    }
    else if ((((((n == COMMAND_VIAL_TOKENS) && (strcmp(tokens[1U], "VIAL") == 0)) &&
                (strlen(tokens[2U]) == 1U)) &&
               (tokens[2U][0U] >= '1')) &&
              (tokens[2U][0U] <= '3')) &&
             (strcmp(tokens[3U], "SENSITIVITY") == 0))
    {
        if (((number(tokens[4U], &value) == 0U) || (value < SENSITIVITY_MIN)) ||
            (value > SENSITIVITY_MAX))
        {
            snprintf(out, size, "ERROR RANGE_OR_FORMAT\r\n");
            return;
        }
        else
        {
            /* No action. */
        }
        a->vials[(tokens[2U][0U] - '1')].sensitivity = value;
    }
    else
    {
        snprintf(out, size, "ERROR COMMAND_OR_ARGUMENTS\r\n");
        return;
    }
    EventLog_Add(now, "CONFIG UPDATED");
    a->last_activity = now;
    snprintf(out, size, "OK RAM ONLY\r\n");
}
