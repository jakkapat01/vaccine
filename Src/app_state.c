#include "app_constants.h"
#include <stdbool.h>
#include "app_state.h"
#include "app_policy.h"
#include "risk_model.h"
#include "event_log.h"
#include "sensor_limits.h"
#include <string.h>

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

static void login(App *a)
{
    memset(a->edit_pin, 0, sizeof(a->edit_pin));
    memset(a->edit_check, 0, sizeof(a->edit_check));
    a->edit_repeat = 0U;
    a->edit_count = 0U;
    a->role = ROLE_NONE;
    PIN_Clear(&a->pin);
    a->state = ST_PIN;
    a->pending_action = 0U;
    a->choice = 0U;
}

static void home(App *a)
{
    login(a);
    if (a->transport_guard != 0U)
    {
        a->state = ST_TRANSPORT;
    }
    else
    {
        a->state = ST_HOME;
    }
}

static void notice(App *a, const char *message)
{
    a->notice = message;
    a->state = ST_NOTICE;
}

void App_Init(App *a, uint32_t now)
{
    memset(a, 0, sizeof(*a));
    Vials_Init(a->vials);
    a->outside_c = OUTSIDE_DEFAULT_C;
    a->last_tick = now;
    a->last_activity = now;
    a->state = ST_HOME;
    a->open_limit_s = OPEN_TIME_DEFAULT_S;
    /* lock_command: 0=no PWM yet, 1=locked command, 2=unlocked command. */
}

void App_Tick(App *a, const AppInput *in)
{
    uint32_t dt = (in->now - a->last_tick);
    /* Charge elapsed time to the preceding sample and lid state. */
    Vials_Update(a->vials, ((float)dt / 1000.0f), a->inside_c, a->temp_valid, a->lid_open,
                 a->light);
    if (((a->temp_valid == 0U) && (dt != 0U)) && (a->state != ST_BOOT))
    {
        a->health_unknown = 1U;
    }
    else
    {
        /* No action. */
    }
    a->sensor_fault = in->sensor_fault;
    if (a->sensor_fault != 0U)
    {
        a->health_unknown = 1U;
    }
    else
    {
        /* No action. */
    }
    a->last_tick = in->now;
    a->inside_c = in->inside_c;
    a->temp_valid = in->temp_valid;
    if (in->light_valid != 0U)
    {
        a->light = in->light;
    }
    else
    {
        a->light = 0U;
    }
    a->risk = Risk_Calculate(a->inside_c, a->outside_c, &a->vials[a->selected], a->temp_valid);
    if ((a->state == ST_COOLDOWN) && (PIN_IsPaused(&a->pin, in->now) == 0U))
    {
        home(a);
    }
    else
    {
        /* No action. */
    }
    if (((a->role != ROLE_NONE) && (a->lock_command == LOCK_CLOSED_COMMAND)) &&
        ((uint32_t)(in->now - a->last_activity) >= SESSION_IDLE_MS))
    {
        home(a);
    }
    else
    {
        /* No action. */
    }
    /* TIM2 interrupt owns open-duration expiry; main consumes its flag. */
}

uint8_t App_AdminConsole(const App *a, uint32_t now)
{
    return ((((a->role == ROLE_ADMIN) && (a->lock_command == LOCK_CLOSED_COMMAND)) &&
             ((a->state == ST_MODE) || (a->state == ST_TRANSPORT))) &&
            ((uint32_t)(now - a->last_activity) < SESSION_IDLE_MS));
}

static void handle_key(App *a, uint8_t key, uint32_t now)
{
    if (a->state == ST_COOLDOWN)
    {
        return;
    }
    else
    {
        /* No action. */
    }
    if (key == KEY_OPEN)
    {
        /* D2 requests entry at HOME; elsewhere it confirms the current step. */
        if ((a->state == ST_CLOSE) || (a->state == ST_OPEN))
        {
            key = KEY_CONFIRM;
        }
        else if ((a->state == ST_HOME) || (a->state == ST_TRANSPORT))
        {
            if (a->hardware_ready == 0U)
            {
                return;
            }
            else
            {
                /* No action. */
            }
            a->lock_command = LOCK_CLOSED_COMMAND;
            login(a);
            a->last_activity = now;
            return;
        }
        else
        {
            key = KEY_CONFIRM;
        }
    }
    else
    {
        /* No action. */
    }
    if ((a->state >= ST_SETTINGS) &&
        (((a->role != ROLE_ADMIN) || (a->lock_command != LOCK_CLOSED_COMMAND)) ||
         (a->transport_guard != 0U)))
    {
        home(a);
        return;
    }
    else
    {
        /* No action. */
    }
    a->last_activity = now;
    if ((key == KEY_LEFT) || (key == KEY_RIGHT))
    {
        unsigned count = 0U;
        if (a->state == ST_SET_RANGE)
        {
            int16_t *v;
            if (a->range_high != 0U)
            {
                v = &a->edit_high;
            }
            else
            {
                v = &a->edit_low;
            }
            int min;
            if (a->range_light != 0U)
            {
                min = LIGHT_PERCENT_MIN;
            }
            else
            {
                min = CONFIG_TEMP_MIN_C;
            }
            int max;
            if (a->range_light != 0U)
            {
                max = LIGHT_PERCENT_MAX;
            }
            else
            {
                max = CONFIG_TEMP_MAX_C;
            }
            if ((key == KEY_RIGHT) && (*v < max))
            {
                ++*v;
            }
            else
            {
                /* No action. */
            }
            if ((key == KEY_LEFT) && (*v > min))
            {
                --*v;
            }
            else
            {
                /* No action. */
            }
            return;
        }
        else
        {
            /* No action. */
        }
        if (a->state == ST_SET_TIME)
        {
            if (key == KEY_RIGHT)
            {
                if (a->edit_seconds > (OPEN_TIME_MAX_S - OPEN_TIME_STEP_S))
                {
                    a->edit_seconds = OPEN_TIME_MAX_S;
                }
                else
                {
                    a->edit_seconds = (a->edit_seconds + OPEN_TIME_STEP_S);
                }
            }
            else
            {
                if (a->edit_seconds < (OPEN_TIME_MIN_S + OPEN_TIME_STEP_S))
                {
                    a->edit_seconds = OPEN_TIME_MIN_S;
                }
                else
                {
                    a->edit_seconds = (uint16_t)(a->edit_seconds - OPEN_TIME_STEP_S);
                }
            }
            return;
        }
        else
        {
            /* No action. */
        }
        if ((a->state == ST_PIN) || (a->state == ST_SET_PIN))
        {
            count = PIN_DIGIT_COUNT;
        }
        else if (a->state == ST_MODE)
        {
            if (a->role == ROLE_ADMIN)
            {
                count = ADMIN_MENU_COUNT;
            }
            else
            {
                count = VIAL_COUNT;
            }
        }
        else if (a->state == ST_VIAL)
        {
            count = VIAL_COUNT;
        }
        else if (a->state == ST_SETTINGS)
        {
            count = SETTINGS_MENU_COUNT;
        }
        else
        {
            /* No action. */
        }
        if (count != 0U)
        {
            if (key == KEY_RIGHT)
            {
                a->choice = (uint8_t)((a->choice + 1U) % count);
            }
            else
            {
                a->choice = (uint8_t)((a->choice + (count - 1U)) % count);
            }
        }
        else
        {
            /* No action. */
        }
        return;
    }
    else
    {
        /* No action. */
    }
    if (key == KEY_BACK)
    {
        switch (a->state)
        {
        case ST_PIN:
            if (a->pin.count != 0U)
            {
                PIN_Back(&a->pin);
            }
            else
            {
                home(a);
            }
            break;
        case ST_SETTINGS:
            a->state = ST_MODE;
            break;
        case ST_SET_RANGE:
            if (a->range_high != 0U)
            {
                a->range_high = 0U;
            }
            else
            {
                a->state = ST_SETTINGS;
            }
            break;
        case ST_SET_TIME:
        case ST_SETTINGS_MSG:
            a->state = ST_SETTINGS;
            break;
        case ST_SET_PIN:
            if (a->edit_count != 0U)
            {
                char *p;
                if (a->edit_repeat != 0U)
                {
                    p = a->edit_check;
                }
                else
                {
                    p = a->edit_pin;
                }
                p[--a->edit_count] = 0;
            }
            else
            {
                memset(a->edit_pin, 0, sizeof(a->edit_pin));
                memset(a->edit_check, 0, sizeof(a->edit_check));
                a->state = ST_SETTINGS;
            }
            break;
        case ST_MODE:
            home(a);
            break;
        case ST_TRANSPORT:
            break;
        case ST_VIAL:
            if (a->role == ROLE_ADMIN)
            {
                a->state = ST_MODE;
            }
            else
            {
                home(a);
            }
            break;
        case ST_NOTICE:
            if (a->role == ROLE_NONE)
            {
                a->state = ST_PIN;
            }
            else
            {
                if (a->role == ROLE_ADMIN)
                {
                    a->state = ST_MODE;
                }
                else
                {
                    a->state = ST_VIAL;
                }
            }
            break;
        case ST_REVIEW:
            a->state = ST_VIAL;
            break;
        case ST_UNLOCKED:
            a->pending_action = 0U;
            a->state = ST_CLOSE;
            break;
        case ST_CLOSE:
            if (a->lid_open != 0U)
            {
                a->pending_action = 0U;
                a->state = ST_OPEN;
            }
            else
            {
                /* No action. */
            }
            break;
        default:
            break;
        }
        return;
    }
    else
    {
        /* No action. */
    }
    if (key != KEY_CONFIRM)
    {
        return;
    }
    else
    {
        /* No action. */
    }
    switch (a->state)
    {
    case ST_BOOT:
        if (a->hardware_ready != 0U)
        {
            a->lock_command = LOCK_CLOSED_COMMAND;
            login(a);
        }
        else
        {
            /* No action. */
        }
        break;
    case ST_PIN:
    {
        Role r;
        if (a->transport_guard != 0U)
        {
            r = PIN_AddRequired(&a->pin, a->choice, now, ROLE_ADMIN);
        }
        else
        {
            r = PIN_AddRequired(&a->pin, a->choice, now, ROLE_NONE);
        }
        if (a->pin.paused != 0U)
        {
            a->state = ST_COOLDOWN;
        }
        else if (r != ROLE_NONE)
        {
            if (a->transport_guard != 0U)
            {
                EventLog_Add(now, "TRANSPORT END ADMIN");
            }
            else
            {
                /* No action. */
            }
            a->transport_guard = 0U;
            a->role = r;
            if (r == ROLE_DISPENSER)
            {
                a->mode = MODE_DISPENSE;
                a->state = ST_VIAL;
            }
            else
            {
                a->state = ST_MODE;
            }
            EventLog_Add(now, select_text((r == ROLE_ADMIN), "LOGIN ADMIN", "LOGIN NURSE"));
        }
        else
        {
            /* No action. */
        }
        break;
    }
    case ST_SETTINGS:
        if (a->choice == 0U)
        {
            a->edit_seconds = (uint16_t)a->open_limit_s;
            a->state = ST_SET_TIME;
        }
        else if (a->choice >= SETTINGS_TEMP_INDEX)
        {
            const SensorLimits *l = SensorLimits_Get();
            a->range_light = (a->choice == SETTINGS_LIGHT_INDEX);
            a->range_high = 0U;
            a->notice = 0;
            if (a->range_light != 0U)
            {
                a->edit_low = l->light_low;
            }
            else
            {
                a->edit_low = l->temp_low;
            }
            if (a->range_light != 0U)
            {
                a->edit_high = l->light_high;
            }
            else
            {
                a->edit_high = l->temp_high;
            }
            a->state = ST_SET_RANGE;
        }
        else
        {
            if (a->choice == 1U)
            {
                a->edit_role = ROLE_DISPENSER;
            }
            else
            {
                a->edit_role = ROLE_ADMIN;
            }
            memset(a->edit_pin, 0, sizeof(a->edit_pin));
            memset(a->edit_check, 0, sizeof(a->edit_check));
            a->edit_repeat = 0U;
            a->edit_count = 0U;
            a->state = ST_SET_PIN;
        }
        break;
    case ST_SET_RANGE:
        if (a->range_high == 0U)
        {
            a->range_high = 1U;
            break;
        }
        else
        {
            /* No action. */
        }
        if (SensorLimits_Set(a->range_light, a->edit_low, a->edit_high) == 0U)
        {
            a->notice = "MIN MUST BE < MAX";
            break;
        }
        else
        {
            /* No action. */
        }
        EventLog_Add(
            now, select_text((a->range_light != 0U), "LIGHT RANGE UPDATED", "TEMP RANGE UPDATED"));
        a->notice = "SAVED (RAM ONLY)";
        a->state = ST_SETTINGS_MSG;
        break;
    case ST_SET_TIME:
        a->open_limit_s = a->edit_seconds;
        EventLog_Add(now, "OLED OPEN TIME UPDATED");
        a->notice = "SAVED (RAM ONLY)";
        a->state = ST_SETTINGS_MSG;
        break;
    case ST_SET_PIN:
    {
        char *p;
        if (a->edit_repeat != 0U)
        {
            p = a->edit_check;
        }
        else
        {
            p = a->edit_pin;
        }
        p[a->edit_count++] = (char)('0' + a->choice);
        if (a->edit_count < PIN_DIGITS)
        {
            break;
        }
        else
        {
            /* No action. */
        }
        if (a->edit_repeat == 0U)
        {
            a->edit_repeat = 1U;
            a->edit_count = 0U;
            a->choice = 0U;
            break;
        }
        else
        {
            /* No action. */
        }
        if (strcmp(a->edit_pin, a->edit_check) != 0)
        {
            a->notice = "PIN MISMATCH";
        }
        else if (PIN_SetCode(a->edit_role, a->edit_pin) == 0U)
        {
            a->notice = "PIN ALREADY USED";
        }
        else
        {
            a->notice = "SAVED (RAM ONLY)";
            EventLog_Add(now, select_text((a->edit_role == ROLE_ADMIN), "OLED ADMIN PIN CHANGED",
                                          "OLED NURSE PIN CHANGED"));
        }
        memset(a->edit_pin, 0, sizeof(a->edit_pin));
        memset(a->edit_check, 0, sizeof(a->edit_check));
        a->edit_count = 0U;
        a->state = ST_SETTINGS_MSG;
        break;
    }
    case ST_SETTINGS_MSG:
        a->state = ST_SETTINGS;
        break;
    case ST_MODE:
        if (a->role != ROLE_ADMIN)
        {
            if (a->role == ROLE_DISPENSER)
            {
                a->mode = MODE_DISPENSE;
                a->state = ST_VIAL;
            }
            else
            {
                home(a);
            }
            break;
        }
        else
        {
            /* No action. */
        }
        if (a->choice == ADMIN_SETTINGS_INDEX)
        {
            if (((a->role == ROLE_ADMIN) && (a->lock_command == LOCK_CLOSED_COMMAND)) &&
                (a->transport_guard == 0U))
            {
                a->state = ST_SETTINGS;
            }
            else
            {
                notice(a, "ADMIN REQUIRED");
            }
            break;
        }
        else
        {
            /* No action. */
        }
        a->mode = (AppMode)a->choice;
        if (a->mode == MODE_TRANSPORT)
        {
            if (a->role != ROLE_ADMIN)
            {
                notice(a, "ADMIN REQUIRED");
            }
            else
            {
                a->transport_guard = 1U;
                a->state = ST_TRANSPORT;
                EventLog_Add(now, "TRANSPORT START ADMIN");
            }
        }
        else if (a->role == ROLE_NONE)
        {
            notice(a, "LOGIN REQUIRED");
        }
        else
        {
            a->state = ST_VIAL;
        }
        break;
    case ST_VIAL:
        a->selected = a->choice;
        if ((a->mode == MODE_DISPENSE) && (a->vials[a->selected].status != VIAL_AVAILABLE))
        {
            notice(a, "VIAL NOT AVAILABLE");
        }
        else
        {
            a->state = ST_REVIEW;
        }
        break;
    case ST_REVIEW:
        a->risk = Risk_Calculate(a->inside_c, a->outside_c, &a->vials[a->selected], a->temp_valid);
        if (a->hardware_ready == 0U)
        {
            notice(a, "HARDWARE FAULT");
            break;
        }
        else
        {
            /* No action. */
        }
        if (a->mode == MODE_DISPENSE)
        {
            if (a->role == ROLE_NONE)
            {
                notice(a, "LOGIN REQUIRED");
                break;
            }
            else
            {
                /* No action. */
            }
            if ((a->sensor_fault != 0U) || (a->temp_valid == 0U))
            {
                notice(a, "SENSOR FAULT");
                break;
            }
            else
            {
                /* No action. */
            }
            if (a->vials[a->selected].status != VIAL_AVAILABLE)
            {
                notice(a, "VIAL NOT AVAILABLE");
                break;
            }
            else
            {
                /* No action. */
            }
            if (a->risk >= DISPENSE_RISK_LIMIT)
            {
                notice(a, "RISK TOO HIGH");
                break;
            }
            else
            {
                /* No action. */
            }
        }
        else if ((a->mode != MODE_SERVICE) || (a->role != ROLE_ADMIN))
        {
            notice(a, "ACCESS DENIED");
            break;
        }
        else
        {
            /* No action. */
        }
        a->lock_command = LOCK_OPEN_COMMAND;
        a->pending_action = 0U;
        a->overdue = 0U;
        a->notice = 0;
        a->state = ST_UNLOCKED;
        break;
    case ST_UNLOCKED:
        break; /* Runtime advances only after a successful servo command. */
    case ST_OPEN:
        a->pending_action = 1U;
        if (((a->pending_action != 0U) && (a->mode == MODE_DISPENSE)) &&
            (a->vials[a->selected].status != VIAL_AVAILABLE))
        {
            a->notice = "QUARANTINED NO TAKE";
            a->pending_action = 0U;
        }
        else
        {
            /* No action. */
        }
        a->state = ST_CLOSE;
        break;
    case ST_CLOSE:
    {
        uint8_t rejected = 0U;
        if (a->pending_action != 0U)
        {
            if ((a->mode == MODE_SERVICE) && (a->role == ROLE_ADMIN))
            {
                Vial_Refill(&a->vials[a->selected]);
                EventLog_Add(now, "REPLACE VIAL CONFIRMED");
            }
            else if (a->mode == MODE_DISPENSE)
            {
                rejected = (Vial_Return(&a->vials[a->selected]) == 0U);
            }
            else
            {
                /* No action. */
            }
        }
        else
        {
            /* No action. */
        }
        a->lid_open = 0U;
        a->lock_command = LOCK_CLOSED_COMMAND;
        a->overdue = 0U;
        home(a);
        if (rejected != 0U)
        {
            notice(a, "QUAR KEEP VIAL");
        }
        else
        {
            /* No action. */
        }
        break;
    }
    case ST_NOTICE:
        if (a->role == ROLE_NONE)
        {
            a->state = ST_PIN;
        }
        else
        {
            if (a->role == ROLE_ADMIN)
            {
                a->state = ST_MODE;
            }
            else
            {
                a->state = ST_VIAL;
            }
        }
        break;
    default:
        break;
    }
}

void App_ServoOpened(App *a, uint32_t now)
{
    if ((a->state == ST_UNLOCKED) && (a->lock_command == LOCK_OPEN_COMMAND))
    {
        a->lid_open = 1U;
        a->open_at = now;
        a->state = ST_OPEN;
    }
    else
    {
        /* No action. */
    }
}

void App_Key(App *a, uint8_t key, uint32_t now)
{
    AppState before = a->state;
    handle_key(a, key, now);
    if (a->state != before)
    {
        if (a->state == ST_VIAL)
        {
            a->choice = a->selected;
        }
        else
        {
            a->choice = 0U;
        }
    }
    else
    {
        /* No action. */
    }
}

const char *App_StateName(AppState s)
{
    static const char *names[] = {"BOOT",         "PIN",      "COOLDOWN", "MODE",     "TRANSPORT",
                                  "VIAL",         "REVIEW",   "UNLOCKED", "OPEN",     "CLOSE",
                                  "NOTICE",       "HOME",     "SETTINGS", "SET_TIME", "SET_PIN",
                                  "SETTINGS_MSG", "SET_RANGE"};
    if ((unsigned)s < (sizeof(names) / sizeof(names[0U])))
    {
        return names[s];
    }
    else
    {
        return "UNKNOWN";
    }
}
