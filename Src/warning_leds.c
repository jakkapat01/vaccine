#include "app_constants.h"
#include "main.h"
#include "warning_leds.h"
/* Training Shield mapping from Exam2 BSP: four active-high LEDs. */
#define LED_A (GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7)
#define LED_B GPIO_PIN_6
static uint8_t active;
static uint8_t on;
static uint32_t changed_at;

static void output(uint8_t value)
{
    if (value != 0U)
    {
        HAL_GPIO_WritePin(GPIOA, LED_A, GPIO_PIN_SET);
    }
    else
    {
        HAL_GPIO_WritePin(GPIOA, LED_A, GPIO_PIN_RESET);
    }
    if (value != 0U)
    {
        HAL_GPIO_WritePin(GPIOB, LED_B, GPIO_PIN_SET);
    }
    else
    {
        HAL_GPIO_WritePin(GPIOB, LED_B, GPIO_PIN_RESET);
    }
}

void WarningLEDs_Init(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    on = 0U;
    active = 0U;
    changed_at = 0U;
    output(0U);
    GPIO_InitTypeDef g = {0U};
    g.Mode = GPIO_MODE_OUTPUT_PP;
    g.Pull = GPIO_NOPULL;
    g.Speed = GPIO_SPEED_FREQ_LOW;
    g.Pin = LED_A;
    HAL_GPIO_Init(GPIOA, &g);
    g.Pin = LED_B;
    HAL_GPIO_Init(GPIOB, &g);
}

void WarningLEDs_Process(uint8_t warning, uint32_t now)
{
    if (warning == 0U)
    {
        if (active != 0U)
        {
            output(0U);
        }
        else
        {
            /* No action. */
        }
        on = 0U;
        active = 0U;
        return;
    }
    else
    {
        /* No action. */
    }
    if (active == 0U)
    {
        on = 1U;
        active = 1U;
        changed_at = now;
        output(1U);
        return;
    }
    else
    {
        /* No action. */
    }
    if ((uint32_t)(now - changed_at) >= DISPLAY_PERIOD_MS)
    {
        changed_at = now;
        on = (on == 0U);
        output(on);
    }
    else
    {
        /* No action. */
    }
}
