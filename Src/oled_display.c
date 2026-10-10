#define OLED_FRAME_BYTES 1024U
#define OLED_GLYPH_COLUMNS 5U
#define OLED_GLYPH_COUNT 36U
#define OLED_DIGIT_GLYPHS 10U
#define OLED_GLYPH_ADVANCE 6U
#define OLED_WIDTH 128U
#define OLED_GLYPH_ROWS 7U
#define OLED_HEIGHT 64U
#define OLED_PAGE_HEIGHT 8U
#define OLED_COMMAND_TIMEOUT_MS 20U
#define OLED_PACKET_BYTES 129U
#define OLED_SET_PAGE_BASE 0xB0U
#define OLED_COLUMN_HIGH 0x10U
#define OLED_DATA_TIMEOUT_MS 30U
#include "app_constants.h"
#include "oled_display.h"
#include <string.h>
extern I2C_HandleTypeDef hi2c1;
#define OLED_SH1106 0
#if OLED_SH1106
#define OLED_COLUMN_OFFSET 0x02U
#else
#define OLED_COLUMN_OFFSET 0x00U
#endif
static uint8_t framebuffer[OLED_FRAME_BYTES];
static uint8_t sent_frame[OLED_FRAME_BYTES];
static uint8_t frame_valid;
static uint16_t oled_address;
static const uint8_t left[OLED_GLYPH_COLUMNS] = {8U, 0x14U, 0x22U, 0x41U, 0U};
static const uint8_t plus[OLED_GLYPH_COLUMNS] = {8U, 8U, 0x3EU, 8U, 8U};
static const uint8_t star[OLED_GLYPH_COLUMNS] = {0x14U, 8U, 0x3EU, 8U, 0x14U};
static const uint8_t arrow[OLED_GLYPH_COLUMNS] = {0U, 0x41U, 0x22U, 0x14U, 8U};
static const uint8_t dot[OLED_GLYPH_COLUMNS] = {0U, 0x60U, 0x60U, 0U, 0U};
static const uint8_t minus[OLED_GLYPH_COLUMNS] = {8U, 8U, 8U, 8U, 8U};
static const uint8_t percent[OLED_GLYPH_COLUMNS] = {0x63U, 0x13U, 8U, 0x64U, 0x63U};

void OLED_Clear(void)
{
    memset(framebuffer, 0, sizeof(framebuffer));
}

/* 5x7 uppercase glyphs, columns, bit 0 at the top. */
static const uint8_t font[OLED_GLYPH_COUNT][OLED_GLYPH_COLUMNS] = {
    {0x3EU, 0x51U, 0x49U, 0x45U, 0x3EU}, {0x00U, 0x42U, 0x7FU, 0x40U, 0x00U},
    {0x42U, 0x61U, 0x51U, 0x49U, 0x46U}, {0x21U, 0x41U, 0x45U, 0x4BU, 0x31U},
    {0x18U, 0x14U, 0x12U, 0x7FU, 0x10U}, {0x27U, 0x45U, 0x45U, 0x45U, 0x39U},
    {0x3CU, 0x4AU, 0x49U, 0x49U, 0x30U}, {0x01U, 0x71U, 0x09U, 0x05U, 0x03U},
    {0x36U, 0x49U, 0x49U, 0x49U, 0x36U}, {0x06U, 0x49U, 0x49U, 0x29U, 0x1EU},
    {0x7EU, 0x11U, 0x11U, 0x11U, 0x7EU}, {0x7FU, 0x49U, 0x49U, 0x49U, 0x36U},
    {0x3EU, 0x41U, 0x41U, 0x41U, 0x22U}, {0x7FU, 0x41U, 0x41U, 0x22U, 0x1CU},
    {0x7FU, 0x49U, 0x49U, 0x49U, 0x41U}, {0x7FU, 0x09U, 0x09U, 0x09U, 0x01U},
    {0x3EU, 0x41U, 0x49U, 0x49U, 0x7AU}, {0x7FU, 0x08U, 0x08U, 0x08U, 0x7FU},
    {0x00U, 0x41U, 0x7FU, 0x41U, 0x00U}, {0x20U, 0x40U, 0x41U, 0x3FU, 0x01U},
    {0x7FU, 0x08U, 0x14U, 0x22U, 0x41U}, {0x7FU, 0x40U, 0x40U, 0x40U, 0x40U},
    {0x7FU, 0x02U, 0x0CU, 0x02U, 0x7FU}, {0x7FU, 0x04U, 0x08U, 0x10U, 0x7FU},
    {0x3EU, 0x41U, 0x41U, 0x41U, 0x3EU}, {0x7FU, 0x09U, 0x09U, 0x09U, 0x06U},
    {0x3EU, 0x41U, 0x51U, 0x21U, 0x5EU}, {0x7FU, 0x09U, 0x19U, 0x29U, 0x46U},
    {0x46U, 0x49U, 0x49U, 0x49U, 0x31U}, {0x01U, 0x01U, 0x7FU, 0x01U, 0x01U},
    {0x3FU, 0x40U, 0x40U, 0x40U, 0x3FU}, {0x1FU, 0x20U, 0x40U, 0x20U, 0x1FU},
    {0x3FU, 0x40U, 0x38U, 0x40U, 0x3FU}, {0x63U, 0x14U, 0x08U, 0x14U, 0x63U},
    {0x07U, 0x08U, 0x70U, 0x08U, 0x07U}, {0x61U, 0x51U, 0x49U, 0x45U, 0x43U}};

void OLED_Text(uint8_t x, uint8_t y, const char *text, uint8_t scale)
{
    while (*text != '\0')
    {
        const uint8_t *glyph = NULL;
        char c = *text++;
        if (c == '<')
        {
            glyph = left;
        }
        else if (c == '+')
        {
            glyph = plus;
        }
        else if (c == '*')
        {
            glyph = star;
        }
        else if (c == '>')
        {
            glyph = arrow;
        }
        else if (c == '.')
        {
            glyph = dot;
        }
        else if (c == '-')
        {
            glyph = minus;
        }
        else if (c == '%')
        {
            glyph = percent;
        }
        else if ((c >= '0') && (c <= '9'))
        {
            glyph = font[(c - '0')];
        }
        else if ((c >= 'A') && (c <= 'Z'))
        {
            glyph = font[((OLED_DIGIT_GLYPHS + c) - 'A')];
        }
        else
        {
            /* No action. */
        }
        if (((unsigned)x + (OLED_GLYPH_ADVANCE * scale)) > OLED_WIDTH)
        {
            break;
        }
        else
        {
            /* No action. */
        }
        if (glyph != NULL)
        {
            for (unsigned col = 0U; (col < OLED_GLYPH_COLUMNS); ++col)
            {
                for (unsigned row = 0U; (row < OLED_GLYPH_ROWS); ++row)
                {
                    if ((glyph[col] & (1U << row)) == 0U)
                    {
                        continue;
                    }
                    else
                    {
                        /* No action. */
                    }
                    for (unsigned dx = 0U; (dx < scale); ++dx)
                    {
                        for (unsigned dy = 0U; (dy < scale); ++dy)
                        {
                            unsigned px = ((x + (col * scale)) + dx);
                            unsigned py = ((y + (row * scale)) + dy);
                            if ((px < OLED_WIDTH) && (py < OLED_HEIGHT))
                            {
                                framebuffer[(((py / OLED_PAGE_HEIGHT) * OLED_WIDTH) + px)] |=
                                    (uint8_t)(1U << (py % OLED_PAGE_HEIGHT));
                            }
                            else
                            {
                                /* No action. */
                            }
                        }
                    }
                }
            }
        }
        else
        {
            /* No action. */
        }
        x = (uint8_t)(x + (OLED_GLYPH_ADVANCE * scale));
    }
}

static HAL_StatusTypeDef command(uint8_t value)
{
    uint8_t bytes[2] = {0x00U, value};
    return HAL_I2C_Master_Transmit(&hi2c1, oled_address, bytes, 2U, OLED_COMMAND_TIMEOUT_MS);
}

uint8_t OLED_Init(void)
{
    frame_valid = 0U;
    const uint8_t addresses[] = {0x3CU, 0x3DU};
    oled_address = 0U;
    for (unsigned i = 0U; (i < sizeof(addresses)); ++i)
    {
        uint16_t candidate = (uint16_t)(addresses[i] << 1U);
        if (HAL_I2C_IsDeviceReady(&hi2c1, candidate, 2U, OLED_COMMAND_TIMEOUT_MS) == HAL_OK)
        {
            oled_address = candidate;
            break;
        }
        else
        {
            /* No action. */
        }
    }
    if (oled_address == 0U)
    {
        return 0U;
    }
    else
    {
        /* No action. */
    }

    const uint8_t init[] = {0xAEU, 0xD5U, 0x80U, 0xA8U, 0x3FU, 0xD3U, 0x00U, 0x40U,
#if OLED_SH1106
                            0xAD,  0x8B,
#else
                            0x8DU, 0x14U, 0x20U, 0x02U, /* SSD1306 page addressing */
#endif
                            0xA1U, 0xC8U, 0xDAU, 0x12U, 0x81U, 0x7FU, 0xD9U, 0x22U,
                            0xDBU, 0x20U, 0xA4U, 0xA6U, 0xAFU};
    for (unsigned i = 0U; (i < sizeof(init)); ++i)
    {
        if (command(init[i]) != HAL_OK)
        {
            return 0U;
        }
        else
        {
            /* No action. */
        }
    }
    return 1U;
}

uint8_t OLED_Update(void)
{
    if (oled_address == 0U)
    {
        return 0U;
    }
    else
    {
        /* No action. */
    }
    if ((frame_valid != 0U) && (memcmp(sent_frame, framebuffer, sizeof(framebuffer)) == 0))
    {
        return 1U;
    }
    else
    {
        /* No action. */
    }
    frame_valid = 0U;
    uint8_t packet[OLED_PACKET_BYTES];
    packet[0U] = OLED_HEIGHT;
    for (unsigned page = 0U; (page < OLED_PAGE_HEIGHT); ++page)
    {
        if (((command((uint8_t)(OLED_SET_PAGE_BASE + page)) != HAL_OK) ||
             (command(OLED_COLUMN_OFFSET) != HAL_OK)) ||
            (command(OLED_COLUMN_HIGH) != HAL_OK))
        {
            return 0U;
        }
        else
        {
            /* No action. */
        }
        memcpy(&packet[1U], &framebuffer[(page * OLED_WIDTH)], OLED_WIDTH);
        if (HAL_I2C_Master_Transmit(&hi2c1, oled_address, packet, OLED_PACKET_BYTES,
                                    OLED_DATA_TIMEOUT_MS) != HAL_OK)
        {
            return 0U;
        }
        else
        {
            /* No action. */
        }
    }
    memcpy(sent_frame, framebuffer, sizeof(framebuffer));
    frame_valid = 1U;
    return 1U;
}

/* Clipped framebuffer drawing: all coordinates are display pixels. */
void OLED_Rect(unsigned x, unsigned y, unsigned w, unsigned h, uint8_t filled)
{
    if ((((x >= OLED_WIDTH) || (y >= OLED_HEIGHT)) || (w == 0U)) || (h == 0U))
    {
        return;
    }
    else
    {
        /* No action. */
    }
    if (w > (OLED_WIDTH - x))
    {
        w = (OLED_WIDTH - x);
    }
    else
    {
        /* No action. */
    }
    if (h > (OLED_HEIGHT - y))
    {
        h = (OLED_HEIGHT - y);
    }
    else
    {
        /* No action. */
    }
    for (unsigned dy = 0U; (dy < h); dy++)
    {
        for (unsigned dx = 0U; (dx < w); dx++)
        {
            if (((((filled != 0U) || (dx == 0U)) || (dy == 0U)) || (dx == (w - 1U))) ||
                (dy == (h - 1U)))
            {
                framebuffer[(((((y + dy) / OLED_PAGE_HEIGHT) * OLED_WIDTH) + x) + dx)] |=
                    (uint8_t)(1U << ((y + dy) % OLED_PAGE_HEIGHT));
            }
            else
            {
                /* No action. */
            }
        }
    }
}
