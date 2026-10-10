#include "app_constants.h"
#define UI_TITLE_BUFFER_SIZE 21U
#define UI_VIAL_LABEL_SIZE 8U
#define UI_VIAL_LABEL_X 12U
#define UI_BAR_HEIGHT 10U
#define UI_QUARANTINE_X 50U
#define UI_ERROR_X 53U
#define UI_BAR_FILL_HEIGHT 6U
#define UI_TEMPERATURE_BUFFER_SIZE 10U
#define UI_SECOND_DETAIL_Y 29U
#define UI_LOCK_STATUS_X 100U
#define UI_REVIEW_TEMPERATURE_Y 13U
#define UI_OPENING_TITLE_Y 21U
#include <stdbool.h>
#include "ui_screen.h"
#include "main.h"
#include "sensors.h"
#include "sensor_limits.h"
#include "oled_display.h"
#include "app_policy.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* Select display/log text without changing either string. */
#define UI_GLYPH_ADVANCE 6U
#define UI_CONTENT_WIDTH 120U
#define UI_DISPLAY_WIDTH 128U
#define UI_MARGIN_X 4U
#define UI_MENU_TEXT_X 21U
#define UI_HEADER_RULE_Y 10U
#define UI_FOOTER_RULE_Y 53U
#define UI_FOOTER_TEXT_Y 56U
#define UI_PERCENT_FULL 100U
#define UI_MARKER_X 8U
#define UI_ROW_OUTLINE_HEIGHT 12U
#define UI_BAR_X 29U
#define UI_BAR_WIDTH 66U
#define UI_HEALTH_ROUNDING 50U
#define UI_BAR_FILL_WIDTH 62U
#define UI_BAR_FILL_X 31U
#define UI_RIGHT_EDGE 124U
#define UI_VIAL_FIRST_Y 14U
#define UI_ROW_SPACING 13U
#define UI_TEXT_BUFFER_SIZE 32U
#define UI_CLOSE_TITLE_Y 19U
#define UI_DETAIL_Y 39U
#define UI_PIN_DIGIT_Y 34U
#define UI_LEFT_KEY_X 20U
#define UI_PIN_ARROWS_Y 38U
#define UI_RIGHT_KEY_X 90U
#define UI_SECOND_ROUNDING_MS 999U
#define UI_COOLDOWN_Y 23U
#define UI_MENU_FIRST_Y 15U
#define UI_RANGE_MIN_Y 16U
#define UI_WARNING_Y 42U
#define UI_TIME_VALUE_Y 18U
#define UI_TIME_HELP_Y 40U
#define UI_NOTICE_Y 24U
#define UI_REVIEW_VIAL_Y 41U
#define UI_ALARM_VALUE_Y 17U

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

static void center(unsigned y, const char *s, unsigned scale)
{
    unsigned width = (((unsigned)strlen(s) * UI_GLYPH_ADVANCE) * scale);
    if (width < UI_CONTENT_WIDTH)
    {
        OLED_Text((uint8_t)((UI_DISPLAY_WIDTH - width) / 2U), (uint8_t)y, s, (uint8_t)scale);
    }
    else
    {
        OLED_Text((uint8_t)UI_MARGIN_X, (uint8_t)y, s, (uint8_t)scale);
    }
}

static void header(const char *s)
{
    char title[UI_TITLE_BUFFER_SIZE];
    if (Sensors_GetFaults(HAL_GetTick()) != 0U)
    {
        snprintf(title, sizeof(title), "FAULT %.13s", s);
        s = title;
    }
    else if (Sensors_TempAlarmActive() != 0U)
    {
        snprintf(title, sizeof(title), "TEMP! %.13s", s);
        s = title;
    }
    else
    {
        /* No action. */
    }
    OLED_Text(UI_MARGIN_X, 0U, s, 1U);
    OLED_Rect(UI_MARGIN_X, UI_HEADER_RULE_Y, UI_CONTENT_WIDTH, 1U, 1U);
}

static void footer(const char *s)
{
    OLED_Rect(UI_MARGIN_X, UI_FOOTER_RULE_Y, UI_CONTENT_WIDTH, 1U, 1U);
    center(UI_FOOTER_TEXT_Y, s, 1U);
}

static void navigation(void)
{
    footer("4< 5> D2:OK D3:BACK");
}

static unsigned health_value(const Vial *v)
{
    if ((isfinite(v->health) == 0) || (v->health <= 0))
    {
        return 0U;
    }
    else
    {
        /* No action. */
    }
    if (v->health >= UI_PERCENT_FULL)
    {
        return UI_PERCENT_FULL;
    }
    else
    {
        /* No action. */
    }
    unsigned h = (unsigned)(v->health + 0.5f);
    if (h != 0U)
    {
        return h;
    }
    else
    {
        return 1U;
    }
}

static void vial_row(const Vial *v, unsigned index, unsigned y, uint8_t selected)
{
    char text[UI_VIAL_LABEL_SIZE];
    unsigned health = health_value(v);
    if (selected != 0U)
    {
        OLED_Text(UI_MARGIN_X, (uint8_t)(y + 1U), ">", 1U);
    }
    else
    {
        /* No action. */
    }
    snprintf(text, sizeof(text), "V%u", (index + 1U));
    OLED_Text(UI_VIAL_LABEL_X, (uint8_t)(y + 1U), text, 1U);
    OLED_Rect(UI_BAR_X, y, UI_BAR_WIDTH, UI_BAR_HEIGHT, 0U);
    if (v->status == VIAL_QUARANTINED)
    {
        OLED_Text(UI_QUARANTINE_X, (uint8_t)(y + 1U), "QUAR", 1U);
    }
    else if (isfinite(v->health) == 0)
    {
        OLED_Text(UI_ERROR_X, (uint8_t)(y + 1U), "ERR", 1U);
    }
    else if (health != 0U)
    {
        unsigned fill = (((UI_BAR_FILL_WIDTH * health) + UI_HEALTH_ROUNDING) / UI_PERCENT_FULL);
        if (fill != 0U)
        {
            OLED_Rect(UI_BAR_FILL_X, (y + 2U), fill, UI_BAR_FILL_HEIGHT, 1U);
        }
        else
        {
            OLED_Rect(UI_BAR_FILL_X, (y + 2U), 1U, UI_BAR_FILL_HEIGHT, 1U);
        }
    }
    else
    {
        /* No action. */
    }
    snprintf(text, sizeof(text), "%u%%", health);
    OLED_Text((uint8_t)(UI_RIGHT_EDGE - (strlen(text) * UI_GLYPH_ADVANCE)), (uint8_t)(y + 1U), text,
              1U);
}

static void vials(const App *a, uint8_t selection)
{
    for (unsigned i = 0U; (i < VIAL_COUNT); i++)
    {
        vial_row(&a->vials[i], i, (UI_VIAL_FIRST_Y + (i * UI_ROW_SPACING)),
                 ((selection != 0U) && (a->choice == i)));
    }
}

static void temperature(char *out, size_t size, float t)
{
    int d = (int)lroundf((t * 10.0f));
    snprintf(out, size, "%s%d.%d", select_text((d < 0), "-", ""), (abs(d) / DECIMAL_BASE),
             (abs(d) % DECIMAL_BASE));
}

uint8_t UI_App(const App *a, uint32_t now)
{
    char text[UI_TEXT_BUFFER_SIZE];
    char in[UI_TEMPERATURE_BUFFER_SIZE];
    char outside[UI_TEMPERATURE_BUFFER_SIZE];
    OLED_Clear();
    switch (a->state)
    {
    case ST_HOME:
        temperature(in, sizeof(in), a->inside_c);
        if ((a->sensor_fault & ((SF_ADC | SF_LIGHT_MASK) | SF_STALE)) != 0)
        {
            snprintf(text, sizeof(text), "%sC LIGHT ERR",
                     select_text((a->temp_valid != 0U), in, "ERR"));
        }
        else
        {
            snprintf(text, sizeof(text), "%sC LIGHT %u%%",
                     select_text((a->temp_valid != 0U), in, "ERR"), (unsigned)a->light);
        }
        header(text);
        vials(a, 0U);
        footer(select_text((a->hardware_ready == 0U), "HARDWARE WAIT",
                           select_text((a->lock_command != LOCK_NO_COMMAND), "D2 REQUEST OPEN",
                                       "CLOSE LID THEN D2")));
        break;
    case ST_BOOT:
        header("NEW SESSION");
        center(UI_CLOSE_TITLE_Y, "CLOSE LID", 2U);
        center(UI_DETAIL_Y, select_text((a->hardware_ready != 0U), "LID SIM", "HARDWARE ERROR"),
               1U);
        footer(select_text((a->hardware_ready != 0U), "D2 CLOSED", "WAIT"));
        break;
    case ST_PIN:
    {
        snprintf(text, sizeof(text),
                 select_text((a->pin.failures != 0U), "PIN  FAIL %u OF 3", "PIN"),
                 (unsigned)a->pin.failures);
        if (a->transport_guard != 0U)
        {
            snprintf(text, sizeof(text), "ADMIN PIN FAIL %u", (unsigned)a->pin.failures);
        }
        else
        {
            /* No action. */
        }
        header(text);
        char mask[PIN_BUFFER_SIZE] = "----";
        for (unsigned i = 0U; (i < a->pin.count); i++)
        {
            mask[i] = '*';
        }
        center(UI_VIAL_FIRST_Y, mask, 2U);
        snprintf(text, sizeof(text), "%u", (unsigned)a->choice);
        center(UI_PIN_DIGIT_Y, text, 2U);
        OLED_Text(UI_LEFT_KEY_X, UI_PIN_ARROWS_Y, "D4<", 1U);
        OLED_Text(UI_RIGHT_KEY_X, UI_PIN_ARROWS_Y, ">D5", 1U);
        footer(select_text((a->pin.count != 0U), "D2:OK  D3:DELETE", "D2:OK  D3:BACK"));
        break;
    }
    case ST_COOLDOWN:
    {
        uint32_t elapsed = (now - a->pin.pause_at);
        unsigned remaining;
        if (elapsed >= AUTH_PAUSE_MS)
        {
            remaining = 0U;
        }
        else
        {
            remaining =
                (((AUTH_PAUSE_MS - elapsed) + UI_SECOND_ROUNDING_MS) / MILLISECONDS_PER_SECOND);
        }
        header("TRY AGAIN IN");
        snprintf(text, sizeof(text), "%u S", remaining);
        center(UI_COOLDOWN_Y, text, 2U);
        footer("MONITORING ACTIVE");
        break;
    }
    case ST_MODE:
    {
        header(select_text((a->role == ROLE_ADMIN), "ADMIN", "NURSE"));
        const char *m[] = {"TRANSPORT", "USE / RETURN", "REPLACE VIAL", "ADMIN SETTINGS"};
        unsigned start;
        if ((a->role == ROLE_ADMIN) && (a->choice == ADMIN_SETTINGS_INDEX))
        {
            start = 1U;
        }
        else
        {
            start = 0U;
        }
        for (unsigned i = 0U; (i < VIAL_COUNT); i++)
        {
            unsigned y = (UI_MENU_FIRST_Y + (i * UI_ROW_SPACING));
            if (a->choice == (i + start))
            {
                OLED_Rect(UI_MARGIN_X, (y - 2U), UI_CONTENT_WIDTH, UI_ROW_OUTLINE_HEIGHT, 0U);
                OLED_Text(UI_MARKER_X, (uint8_t)y, ">", 1U);
            }
            else
            {
                /* No action. */
            }
            OLED_Text(UI_MENU_TEXT_X, (uint8_t)y, m[(i + start)], 1U);
        }
        navigation();
        break;
    }
    case ST_SETTINGS:
    {
        header("ADMIN SETTINGS");
        const char *items[] = {"OPEN TIME", "NURSE PIN", "ADMIN PIN", "TEMP RANGE", "LIGHT RANGE"};
        unsigned start;
        if (a->choice > 2U)
        {
            start = (a->choice - 2U);
        }
        else
        {
            start = 0U;
        }
        for (unsigned i = 0U; (i < VIAL_COUNT); i++)
        {
            unsigned y = (UI_MENU_FIRST_Y + (i * UI_ROW_SPACING));
            if (a->choice == (i + start))
            {
                OLED_Rect(UI_MARGIN_X, (y - 2U), UI_CONTENT_WIDTH, UI_ROW_OUTLINE_HEIGHT, 0U);
                OLED_Text(UI_MARKER_X, (uint8_t)y, ">", 1U);
            }
            else
            {
                /* No action. */
            }
            OLED_Text(UI_MENU_TEXT_X, (uint8_t)y, items[(i + start)], 1U);
        }
        navigation();
        break;
    }
    case ST_SET_RANGE:
        header(select_text((a->range_light != 0U), "LIGHT RANGE %", "TEMP RANGE C"));
        snprintf(text, sizeof(text), "%s MIN %d", select_text((a->range_high != 0U), " ", ">"),
                 a->edit_low);
        center(UI_RANGE_MIN_Y, text, 1U);
        snprintf(text, sizeof(text), "%s MAX %d", select_text((a->range_high != 0U), ">", " "),
                 a->edit_high);
        center(UI_SECOND_DETAIL_Y, text, 1U);
        if (a->notice != 0)
        {
            center(UI_WARNING_Y, a->notice, 1U);
        }
        else
        {
            /* No action. */
        }
        footer(
            select_text((a->range_high != 0U), "4- 5+ D2:SAVE D3:BACK", "4- 5+ D2:NEXT D3:BACK"));
        break;
    case ST_SET_TIME:
        header("OPEN TIME");
        snprintf(text, sizeof(text), "%u SEC", (unsigned)a->edit_seconds);
        center(UI_TIME_VALUE_Y, text, 2U);
        center(UI_TIME_HELP_Y, "10-300 / STEP 5", 1U);
        footer("4- 5+ D2:SAVE D3:BACK");
        break;
    case ST_SET_PIN:
    {
        header(select_text(
            (a->edit_repeat != 0U), "REPEAT NEW PIN",
            select_text((a->edit_role == ROLE_ADMIN), "NEW ADMIN PIN", "NEW NURSE PIN")));
        char mask[PIN_BUFFER_SIZE] = "----";
        for (unsigned i = 0U; (i < a->edit_count); i++)
        {
            mask[i] = '*';
        }
        center(UI_VIAL_FIRST_Y, mask, 2U);
        snprintf(text, sizeof(text), "%u", (unsigned)a->choice);
        center(UI_PIN_DIGIT_Y, text, 2U);
        OLED_Text(UI_LEFT_KEY_X, UI_PIN_ARROWS_Y, "D4<", 1U);
        OLED_Text(UI_RIGHT_KEY_X, UI_PIN_ARROWS_Y, ">D5", 1U);
        footer(select_text((a->edit_count != 0U), "D2:OK  D3:DELETE", "D2:OK  D3:BACK"));
        break;
    }
    case ST_SETTINGS_MSG:
        header("ADMIN SETTINGS");
        center(UI_NOTICE_Y, select_text((a->notice != 0), a->notice, ""), 1U);
        footer("D2:OK  D3:BACK");
        break;
    case ST_TRANSPORT:
        temperature(in, sizeof(in), a->inside_c);
        snprintf(text, sizeof(text), "IN %sC", select_text((a->temp_valid != 0U), in, "--"));
        header(text);
        OLED_Text(UI_LOCK_STATUS_X, 0U, select_text((a->temp_valid != 0U), "LOCK", "ERR"), 1U);
        vials(a, 0U);
        footer(select_text((a->health_unknown != 0U), "GAP  D2 ADMIN EXIT", "D2 ADMIN EXIT"));
        break;
    case ST_VIAL:
        header(select_text((a->mode == MODE_SERVICE), "REPLACE VIAL", "PICK VIAL"));
        vials(a, 1U);
        navigation();
        break;
    case ST_REVIEW:
        snprintf(text, sizeof(text),
                 select_text((a->mode == MODE_SERVICE), "REPLACE V%u", "DISPENSE V%u"),
                 (a->selected + 1U));
        header(text);
        temperature(in, sizeof(in), a->inside_c);
        temperature(outside, sizeof(outside), a->outside_c);
        snprintf(text, sizeof(text), "IN%s SIM%s", select_text((a->temp_valid != 0U), in, "ERR"),
                 outside);
        center(UI_REVIEW_TEMPERATURE_Y, text, 1U);
        snprintf(text, sizeof(text), "RISK %u", (unsigned)a->risk);
        center(UI_NOTICE_Y, text, 2U);
        vial_row(&a->vials[a->selected], a->selected, UI_REVIEW_VIAL_Y, 0U);
        footer(select_text((a->mode == MODE_SERVICE), "D2:REPLACE D3:BACK", "D2:OPEN  D3:BACK"));
        break;
    case ST_UNLOCKED:
        header("SERVO");
        center(UI_OPENING_TITLE_Y, "OPENING", 2U);
        footer(select_text((a->hardware_ready != 0U), "PLEASE WAIT", "SERVO ERROR"));
        break;
    case ST_OPEN:
        if (a->overdue != 0U)
        {
            header("TIMEOUT - CLOSE LID");
        }
        else
        {
            snprintf(text, sizeof(text), "OPEN %luS",
                     (unsigned long)((now - a->open_at) / MILLISECONDS_PER_SECOND));
            header(text);
        }
        vials(a, 0U);
        footer("D2 FINISH");
        break;
    case ST_CLOSE:
        snprintf(text, sizeof(text), "V%u %s", (a->selected + 1U),
                 select_text((a->pending_action != 0U),
                             select_text((a->mode == MODE_SERVICE), "NEW VIAL", "RETURN VIAL"),
                             "CANCELLED"));
        header(text);
        center(UI_CLOSE_TITLE_Y, "CLOSE LID", 2U);
        center(UI_DETAIL_Y,
               select_text((a->notice != 0), "QUAR - KEEP VIAL",
                           select_text((a->overdue != 0U), "TIMEOUT", "LID SIM")),
               1U);
        footer("D2:CLOSED D3:BACK");
        break;
    case ST_NOTICE:
        header("NO ACCESS");
        center(UI_NOTICE_Y, select_text((a->notice != 0), a->notice, "ERROR"), 1U);
        footer("D3 BACK");
        break;

    default:
    { /* Preserve the existing no-action fallback. */
        break;
    }
    }
    return OLED_Update();
}

uint8_t UI_TempAlarm(int16_t t, uint8_t valid)
{
    char text[UI_TEXT_BUFFER_SIZE];
    OLED_Clear();
    header("TEMP ALARM");
    if (valid != 0U)
    {
        snprintf(text, sizeof(text), "%s%d.%d C", select_text((t < 0), "-", ""),
                 (abs(t) / DECIMAL_BASE), (abs(t) % DECIMAL_BASE));
    }
    else
    {
        snprintf(text, sizeof(text), "SENSOR ERROR");
    }
    if (valid != 0U)
    {
        center(UI_ALARM_VALUE_Y, text, 2U);
    }
    else
    {
        center(UI_ALARM_VALUE_Y, text, 1U);
    }
    snprintf(text, sizeof(text), "LIMIT %d TO %d C", SensorLimits_Get()->temp_low,
             SensorLimits_Get()->temp_high);
    center(UI_DETAIL_Y, text, 1U);
    footer("D3 ACK / RESUME");
    return OLED_Update();
}

uint8_t UI_LightAlarm(uint8_t light)
{
    char text[UI_TEXT_BUFFER_SIZE];
    OLED_Clear();
    header("LIGHT ALARM");
    snprintf(text, sizeof(text), "%u%%", light);
    center(UI_ALARM_VALUE_Y, text, 2U);
    snprintf(text, sizeof(text), "LIMIT %d TO %d %%", SensorLimits_Get()->light_low,
             SensorLimits_Get()->light_high);
    center(UI_DETAIL_Y, text, 1U);
    footer("D3 ACK / RESUME");
    return OLED_Update();
}

uint8_t UI_SensorFault(uint8_t f)
{
    OLED_Clear();
    header("SENSOR FAULT");
    if ((f & SF_ADC) != 0)
    {
        center(UI_RANGE_MIN_Y, "ADC INIT ERROR", 1U);
    }
    else if ((f & SF_STALE) != 0)
    {
        center(UI_RANGE_MIN_Y, "DATA STALE >2S", 1U);
    }
    else if ((f & SF_TEMP_READ) != 0)
    {
        center(UI_RANGE_MIN_Y, "TEMP READ FAIL", 1U);
    }
    else if ((f & SF_TEMP_RAIL) != 0)
    {
        center(UI_RANGE_MIN_Y, "TEMP ADC RAIL", 1U);
    }
    else if ((f & SF_TEMP_RANGE) != 0)
    {
        center(UI_RANGE_MIN_Y, "TEMP INVALID", 1U);
    }
    else
    {
        /* No action. */
    }
    if ((f & SF_LIGHT_READ) != 0)
    {
        center(UI_SECOND_DETAIL_Y, "LIGHT READ FAIL", 1U);
    }
    else if ((f & SF_LIGHT_RAIL) != 0)
    {
        center(UI_SECOND_DETAIL_Y, "LIGHT ADC RAIL", 1U);
    }
    else
    {
        /* No action. */
    }
    center(UI_WARNING_Y, "CHECK SENSOR/WIRE", 1U);
    footer("D3 ACK / USE BLOCK");
    return OLED_Update();
}
