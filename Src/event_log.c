#include "app_constants.h"
#include "event_log.h"
#include <stdio.h>
#include <string.h>

static struct
{
    uint32_t ms;
    uint32_t seq;
    char text[LOG_TEXT_SIZE];
} rows[LOG_CAPACITY];

static unsigned head;
static unsigned count;
static uint32_t sequence;

void EventLog_Init(void)
{
    count = 0U;
    head = 0U;
    sequence = 0U;
}

void EventLog_Add(uint32_t now, const char *event)
{
    rows[head].ms = now;
    rows[head].seq = ++sequence;
    snprintf(rows[head].text, sizeof(rows[head].text), "%s", event);
    head = ((head + 1U) % LOG_CAPACITY);
    if (count < LOG_CAPACITY)
    {
        count++;
    }
    else
    {
        /* No action. */
    }
}

void EventLog_Read(unsigned page, char *out, size_t size)
{
    if (size == 0U)
    {
        return;
    }
    else
    {
        /* No action. */
    }
    snprintf(out, size, "LOG RAM page %u/3 newest first; resets on reboot\r\n", page);
    for (unsigned j = (page * LOG_PAGE_ROWS);
         ((j < count) && (j < ((page * LOG_PAGE_ROWS) + LOG_PAGE_ROWS))); j++)
    {
        unsigned i = (((head + LOG_LAST_OFFSET) - j) % LOG_CAPACITY);
        size_t used = strlen(out);
        if (used >= (size - 1U))
        {
            break;
        }
        else
        {
            /* No action. */
        }
        snprintf((out + used), (size - used), "#%lu %lums %s\r\n", (unsigned long)rows[i].seq,
                 (unsigned long)rows[i].ms, rows[i].text);
    }
}
