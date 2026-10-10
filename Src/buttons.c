#include "app_constants.h"
#include "main.h"
#include "buttons.h"
#include "app_state.h"
/* All four EXTI IRQs share priority 5: one producer, main is the consumer.
 * Completed presses are queued on release, never execute application code here. */
static GPIO_TypeDef *const ports[BUTTON_COUNT] = {GPIOA, GPIOB, GPIOB, GPIOB};
static const uint16_t pins[BUTTON_COUNT] = {GPIO_PIN_10, GPIO_PIN_3, GPIO_PIN_5, GPIO_PIN_4};
static const IRQn_Type irqs[BUTTON_COUNT] = {EXTI15_10_IRQn, EXTI3_IRQn, EXTI9_5_IRQn, EXTI4_IRQn};
static volatile uint32_t irq_count[BUTTON_COUNT];
static volatile uint32_t dropped;
static uint32_t pressed_at[BUTTON_COUNT];
static uint32_t accepted_at[BUTTON_COUNT];
static uint8_t down;
static uint8_t blocked;
static uint8_t ready;
static volatile uint8_t open_request;
static volatile uint8_t queue[BUTTON_QUEUE_SIZE];
static volatile uint8_t head;
static volatile uint8_t tail;

void Buttons_Init(void)
{
    GPIO_InitTypeDef g = {0U};
    ready = 0U;
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_SYSCFG_CLK_ENABLE();
    for (unsigned i = 0U; (i < BUTTON_COUNT); i++)
    {
        HAL_NVIC_DisableIRQ(irqs[i]);
    }
    g.Pull = GPIO_PULLUP;
    g.Mode = GPIO_MODE_IT_RISING_FALLING;
    g.Pin = GPIO_PIN_10;
    HAL_GPIO_Init(GPIOA, &g);
    g.Pin = ((GPIO_PIN_3 | GPIO_PIN_4) | GPIO_PIN_5);
    HAL_GPIO_Init(GPIOB, &g);
    down = 0U;
    tail = 0U;
    head = 0U;
    dropped = 0U;
    open_request = 0U;
    for (unsigned i = 0U; (i < BUTTON_COUNT); i++)
    {
        irq_count[i] = 0U;
        pressed_at[i] = HAL_GetTick();
        accepted_at[i] = (pressed_at[i] - BUTTON_DEBOUNCE_MS);
        if (HAL_GPIO_ReadPin(ports[i], pins[i]) == GPIO_PIN_RESET)
        {
            down |= (1U << i);
        }
        else
        {
            /* No action. */
        }
        __HAL_GPIO_EXTI_CLEAR_IT(pins[i]);
        HAL_NVIC_ClearPendingIRQ(irqs[i]);
        HAL_NVIC_SetPriority(irqs[i], APP_IRQ_PRIORITY, 0U);
    }
    /* A key held during boot must be released before it can act. */
    blocked = down;
    ready = 1U;
    for (unsigned i = 0U; (i < BUTTON_COUNT); i++)
    {
        HAL_NVIC_EnableIRQ(irqs[i]);
    }
}

void Buttons_Edge(uint16_t pin)
{
    if (ready == 0U)
    {
        return;
    }
    else
    {
        /* No action. */
    }
    for (unsigned i = 0U; (i < BUTTON_COUNT); i++)
    {
        if (pin == pins[i])
        {
            irq_count[i]++;
            uint32_t now = HAL_GetTick();
            uint8_t bit = (uint8_t)(1U << i);
            if (HAL_GPIO_ReadPin(ports[i], pin) == GPIO_PIN_RESET)
            {
                if ((down & bit) == 0)
                {
                    pressed_at[i] = now;
                }
                else
                {
                    /* No action. */
                }
                down |= bit;
                if ((down & (down - 1U)) != 0U)
                {
                    blocked |= down; /* Suppress overlapping keys. */
                }
                else
                {
                    /* No action. */
                }
            }
            else
            {
                if (((((down & bit) != 0) && ((blocked & bit) == 0)) &&
                     ((uint32_t)(now - pressed_at[i]) >= BUTTON_DEBOUNCE_MS)) &&
                    ((uint32_t)(now - accepted_at[i]) >= BUTTON_DEBOUNCE_MS))
                {
                    uint8_t next = ((head + 1U) & BUTTON_QUEUE_MASK);
                    if (i == 0U)
                    {
                        open_request = 1U;
                    }
                    else if (next == tail)
                    {
                        dropped++;
                    }
                    else
                    {
                        queue[head] = bit;
                        __DMB();
                        head = next;
                    }
                    accepted_at[i] = now;
                }
                else
                {
                    /* No action. */
                }
                down &= (uint8_t)~bit;
                blocked &= (uint8_t)~bit;
            }
            return;
        }
        else
        {
            /* No action. */
        }
    }
}

uint8_t Buttons_TakeOpenRequest(void)
{
    uint32_t p = __get_PRIMASK();
    __disable_irq();
    uint8_t result = open_request;
    open_request = 0U;
    if (result != 0U)
    {
        tail = head; /* Discard stale navigation when a global request arrives. */
    }
    else
    {
        /* No action. */
    }
    __set_PRIMASK(p);
    return result;
}

uint8_t Buttons_GetEvent(void)
{
    if (tail == head)
    {
        return 0U;
    }
    else
    {
        /* No action. */
    }
    __DMB();
    uint8_t event = queue[tail];
    __DMB();
    tail = ((tail + 1U) & BUTTON_QUEUE_MASK);
    return event;
}

void Buttons_GetIrqStats(uint32_t counts[BUTTON_COUNT], uint32_t *lost)
{
    uint32_t p = __get_PRIMASK();
    __disable_irq();
    for (unsigned i = 0U; (i < BUTTON_COUNT); i++)
    {
        counts[i] = irq_count[i];
    }
    *lost = dropped;
    __set_PRIMASK(p);
}
