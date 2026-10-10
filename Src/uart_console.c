#include "app_constants.h"
#include "sensors.h"
#include "lid_timer.h"
#include "main.h"
#include "uart_console.h"
#include "console_commands.h"
#include <string.h>
#include <stdio.h>
#include "buttons.h"
/* USART2 PA2/PA3 to ST-LINK virtual COM. 115200 8N1. */
#define ASCII_DELETE 127
#define ASCII_SPACE 32
#define ASCII_PRINTABLE_LAST 126
static volatile uint8_t rx[SERIAL_RX_SIZE];
static volatile uint16_t rh;
static volatile uint16_t rt;
static volatile uint8_t rx_overflow;
static char tx[SERIAL_TX_SIZE];
static uint16_t th;
static uint16_t tt;
static char line[SERIAL_LINE_SIZE];
static unsigned used;
static uint8_t dropping;

void UARTConsole_Init(void)
{
    GPIO_InitTypeDef g = {0U};
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_USART2_CLK_ENABLE();
    g.Pin = (GPIO_PIN_2 | GPIO_PIN_3);
    g.Mode = GPIO_MODE_AF_PP;
    g.Pull = GPIO_PULLUP;
    g.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    g.Alternate = GPIO_AF7_USART2;
    HAL_GPIO_Init(GPIOA, &g);
    USART2->CR1 = 0U;
    USART2->CR2 = 0U;
    USART2->CR3 = 0U;
    USART2->BRR = ((HAL_RCC_GetPCLK1Freq() + SERIAL_HALF_BAUD) / SERIAL_BAUD);
    USART2->CR1 = (((USART_CR1_UE | USART_CR1_TE) | USART_CR1_RE) | USART_CR1_RXNEIE);
    HAL_NVIC_SetPriority(USART2_IRQn, UART_IRQ_PRIORITY, 0U);
    HAL_NVIC_EnableIRQ(USART2_IRQn);
    UARTConsole_Log("VACCINE CLASSROOM MODEL - RAM RESET\r\nIN REAL SENSOR / OUT SIM CONSOLE / LID "
                    "SIM D2\r\n115200 8N1; HELP for commands\r\n");
}

void UARTConsole_IRQHandler(void)
{
    uint32_t sr = USART2->SR;
    if ((sr & ((((USART_SR_RXNE | USART_SR_ORE) | USART_SR_NE) | USART_SR_FE) | USART_SR_PE)) != 0U)
    {
        uint8_t c = (uint8_t)USART2->DR;
        if ((sr & (((USART_SR_ORE | USART_SR_NE) | USART_SR_FE) | USART_SR_PE)) != 0U)
        {
            rx_overflow = 1U;
            return;
        }
        else
        {
            /* No action. */
        }
        uint16_t next = ((rh + 1U) & SERIAL_RX_MASK);
        if (next == rt)
        {
            rx_overflow = 1U;
        }
        else
        {
            rx[rh] = c;
            rh = next;
        }
    }
    else
    {
        /* No action. */
    }
}

void UARTConsole_Log(const char *s)
{
    unsigned length = (unsigned)strlen(s);
    unsigned space = (((tt - th) - 1U) & SERIAL_TX_MASK);
    if (length > space)
    {
        return; /* Drop whole diagnostic, never partial reply. */
    }
    else
    {
        /* No action. */
    }
    while (*s != 0)
    {
        tx[th] = *s++;
        th = ((th + 1U) & SERIAL_TX_MASK);
    }
}

void UARTConsole_Process(App *a)
{
    if (rx_overflow != 0U)
    {
        uint32_t p = __get_PRIMASK();
        __disable_irq();
        rt = rh;
        rx_overflow = 0U;
        __set_PRIMASK(p);
        dropping = 1U;
        used = 0U;
    }
    else
    {
        /* No action. */
    }
    unsigned budget = SERIAL_WORK_BUDGET;
    while ((rt != rh) && (budget-- != 0U))
    {
        /* Reserve room for an entire command reply before executing a command. */
        if ((((tt - th) - 1U) & SERIAL_TX_MASK) < SERIAL_REPLY_SIZE)
        {
            break;
        }
        else
        {
            /* No action. */
        }
        char c = (char)rx[rt];
        rt = ((rt + 1U) & SERIAL_RX_MASK);
        if ((c == '\r') || (c == '\n'))
        {
            if (dropping != 0U)
            {
                UARTConsole_Log("ERROR LINE_OVERFLOW\r\n");
                dropping = 0U;
                used = 0U;
            }
            else if (used != 0U)
            {
                char reply[SERIAL_REPLY_SIZE];
                line[used] = 0;
                if (strcmp(line, "IRQ") == 0)
                {
                    uint32_t counts[BUTTON_COUNT];
                    uint32_t lost;
                    Buttons_GetIrqStats(counts, &lost);
                    snprintf(reply, sizeof(reply),
                             "IRQ EDGES D2=%lu D3=%lu D4=%lu D5=%lu DROPPED=%lu ADC_AWD=%lu "
                             "TEMP_ALARM=%u TIM2_EXPIRED=%lu LIGHT_EOC=%lu LIGHT_ALARM_COUNT=%lu "
                             "LIGHT_ALARM=%u\r\n",
                             (unsigned long)counts[0U], (unsigned long)counts[1U],
                             (unsigned long)counts[2U], (unsigned long)counts[3U],
                             (unsigned long)lost, (unsigned long)Sensors_TempAlarmCount(),
                             (unsigned)Sensors_TempAlarmActive(), (unsigned long)LidTimer_Count(),
                             (unsigned long)Sensors_LightEocCount(),
                             (unsigned long)Sensors_LightAlarmCount(),
                             (unsigned)Sensors_LightAlarmActive());
                }
                else
                {
                    Console_Command(a, line, HAL_GetTick(), reply, sizeof(reply));
                }
                UARTConsole_Log(reply);
                used = 0U;
            }
            else
            {
                /* No action. */
            }
        }
        else if (dropping == 0U)
        {
            if ((c == '\b') || (c == ASCII_DELETE))
            {
                if (used != 0U)
                {
                    used--;
                }
                else
                {
                    /* No action. */
                }
            }
            else if ((c == '\t') || ((c >= ASCII_SPACE) && (c <= ASCII_PRINTABLE_LAST)))
            {
                if (used < (sizeof(line) - 1U))
                {
                    line[used++] = c;
                }
                else
                {
                    dropping = 1U;
                    used = 0U;
                }
            }
            else
            {
                dropping = 1U;
                used = 0U;
            }
        }
        else
        {
            /* No action. */
        }
    }
    budget = SERIAL_WORK_BUDGET;
    while (((tt != th) && (budget-- != 0U)) && ((USART2->SR & USART_SR_TXE) != 0U))
    {
        USART2->DR = (uint8_t)tx[tt];
        tt = ((tt + 1U) & SERIAL_TX_MASK);
    }
}
