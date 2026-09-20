#include "oled_display.h"
#include <string.h>
extern I2C_HandleTypeDef hi2c1;
#define OLED_SH1106 0
static uint8_t framebuffer[1024];
static uint8_t sent_frame[1024];
static uint8_t frame_valid;
static uint16_t oled_address;
static const uint8_t left[5]={8,0x14,0x22,0x41,0};
static const uint8_t plus[5]={8,8,0x3E,8,8};
static const uint8_t star[5]={0x14,8,0x3E,8,0x14};
static const uint8_t arrow[5]={0,0x41,0x22,0x14,8};
static const uint8_t dot[5]={0,0x60,0x60,0,0};
static const uint8_t minus[5]={8,8,8,8,8};
static const uint8_t percent[5]={0x63,0x13,8,0x64,0x63};
void OLED_Clear(void) { memset(framebuffer,0,sizeof(framebuffer)); }
/* 5x7 uppercase glyphs, columns, bit 0 at the top. */
static const uint8_t font[36][5] = {
    {0x3E,0x51,0x49,0x45,0x3E}, {0x00,0x42,0x7F,0x40,0x00},
    {0x42,0x61,0x51,0x49,0x46}, {0x21,0x41,0x45,0x4B,0x31},
    {0x18,0x14,0x12,0x7F,0x10}, {0x27,0x45,0x45,0x45,0x39},
    {0x3C,0x4A,0x49,0x49,0x30}, {0x01,0x71,0x09,0x05,0x03},
    {0x36,0x49,0x49,0x49,0x36}, {0x06,0x49,0x49,0x29,0x1E},
    {0x7E,0x11,0x11,0x11,0x7E}, {0x7F,0x49,0x49,0x49,0x36},
    {0x3E,0x41,0x41,0x41,0x22}, {0x7F,0x41,0x41,0x22,0x1C},
    {0x7F,0x49,0x49,0x49,0x41}, {0x7F,0x09,0x09,0x09,0x01},
    {0x3E,0x41,0x49,0x49,0x7A}, {0x7F,0x08,0x08,0x08,0x7F},
    {0x00,0x41,0x7F,0x41,0x00}, {0x20,0x40,0x41,0x3F,0x01},
    {0x7F,0x08,0x14,0x22,0x41}, {0x7F,0x40,0x40,0x40,0x40},
    {0x7F,0x02,0x0C,0x02,0x7F}, {0x7F,0x04,0x08,0x10,0x7F},
    {0x3E,0x41,0x41,0x41,0x3E}, {0x7F,0x09,0x09,0x09,0x06},
    {0x3E,0x41,0x51,0x21,0x5E}, {0x7F,0x09,0x19,0x29,0x46},
    {0x46,0x49,0x49,0x49,0x31}, {0x01,0x01,0x7F,0x01,0x01},
    {0x3F,0x40,0x40,0x40,0x3F}, {0x1F,0x20,0x40,0x20,0x1F},
    {0x3F,0x40,0x38,0x40,0x3F}, {0x63,0x14,0x08,0x14,0x63},
    {0x07,0x08,0x70,0x08,0x07}, {0x61,0x51,0x49,0x45,0x43}
};

void OLED_Text(uint8_t x, uint8_t y, const char *text, uint8_t scale)
{
    while (*text != '\0') {
        const uint8_t *glyph = NULL;
        char c = *text++;
        if (c == '<') glyph = left;
        else if (c == '+') glyph = plus;
        else if (c == '*') glyph = star;
        else if (c == '>') glyph = arrow;
        else if (c == '.') glyph = dot;
        else if (c == '-') glyph = minus;
        else if (c == '%') glyph = percent;
        else if (c >= '0' && c <= '9') glyph = font[c - '0'];
        else if (c >= 'A' && c <= 'Z') glyph = font[10 + c - 'A'];
        if ((unsigned)x + 6U * scale > 128U) break;
        if (glyph != NULL) {
            for (unsigned col = 0; col < 5U; ++col) {
                for (unsigned row = 0; row < 7U; ++row) {
                    if ((glyph[col] & (1U << row)) == 0U) continue;
                    for (unsigned dx = 0; dx < scale; ++dx) {
                        for (unsigned dy = 0; dy < scale; ++dy) {
                            unsigned px = x + col * scale + dx;
                            unsigned py = y + row * scale + dy;
                            if (px < 128U && py < 64U)
                                framebuffer[(py / 8U) * 128U + px] |= (uint8_t)(1U << (py % 8U));
                        }
                    }
                }
            }
        }
        x = (uint8_t)(x + 6U * scale);
    }
}

static HAL_StatusTypeDef command(uint8_t value)
{
    uint8_t bytes[2] = {0x00U, value};
    return HAL_I2C_Master_Transmit(&hi2c1, oled_address, bytes, 2U, 20U);
}

uint8_t OLED_Init(void)
{
    frame_valid=0;
    const uint8_t addresses[] = {0x3CU, 0x3DU};
    oled_address = 0U;
    for (unsigned i = 0; i < sizeof(addresses); ++i) {
        uint16_t candidate = (uint16_t)(addresses[i] << 1);
        if (HAL_I2C_IsDeviceReady(&hi2c1, candidate, 2U, 20U) == HAL_OK) {
            oled_address = candidate;
            break;
        }
    }
    if (oled_address == 0U) return 0U;

    const uint8_t init[] = {
        0xAE, 0xD5, 0x80, 0xA8, 0x3F, 0xD3, 0x00, 0x40,
#if OLED_SH1106
        0xAD, 0x8B,
#else
        0x8D, 0x14, 0x20, 0x02, /* SSD1306 page addressing */
#endif
        0xA1, 0xC8, 0xDA, 0x12, 0x81, 0x7F,
        0xD9, 0x22, 0xDB, 0x20, 0xA4, 0xA6, 0xAF
    };
    for (unsigned i = 0; i < sizeof(init); ++i) {
        if (command(init[i]) != HAL_OK) return 0U;
    }
    return 1U;
}

uint8_t OLED_Update(void)
{
    if (oled_address == 0U) return 0U;
    if(frame_valid && !memcmp(sent_frame,framebuffer,sizeof(framebuffer))) return 1U;
    frame_valid=0;
    uint8_t packet[129];
    packet[0] = 0x40U;
    for (unsigned page = 0; page < 8U; ++page) {
        if (command((uint8_t)(0xB0U + page)) != HAL_OK ||
            command(OLED_SH1106 ? 0x02U : 0x00U) != HAL_OK ||
            command(0x10U) != HAL_OK) return 0U;
        memcpy(&packet[1], &framebuffer[page * 128U], 128U);
        if (HAL_I2C_Master_Transmit(&hi2c1, oled_address, packet, 129U, 30U) != HAL_OK)
            return 0U;
    }
    memcpy(sent_frame,framebuffer,sizeof(framebuffer));
    frame_valid=1;
    return 1U;
}


/* Clipped framebuffer drawing: all coordinates are display pixels. */
void OLED_Rect(unsigned x,unsigned y,unsigned w,unsigned h,uint8_t filled)
{
    if(x>=128U || y>=64U || w==0U || h==0U) return;
    if(w>128U-x) w=128U-x;
    if(h>64U-y) h=64U-y;
    for(unsigned dy=0;dy<h;dy++) for(unsigned dx=0;dx<w;dx++) {
        if(filled || dx==0U || dy==0U || dx==w-1U || dy==h-1U)
            framebuffer[((y+dy)/8U)*128U+x+dx]|=(uint8_t)(1U<<((y+dy)%8U));
    }
}
