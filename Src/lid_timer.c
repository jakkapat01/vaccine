#include "app_constants.h"
#include "main.h"
#include "lid_timer.h"
#define APB_TIMER_CLOCK_MULTIPLIER 2U
#define LID_TIMER_TICKS_PER_SECOND 10000U
#define LID_TIMER_PERIOD 9999U
static volatile uint32_t remaining;
static volatile uint32_t expirations;
static volatile uint8_t expired;

void LidTimer_Init(void)
{
    __HAL_RCC_TIM2_CLK_ENABLE();
    TIM2->CR1 = 0U;
    TIM2->DIER = 0U;
    uint32_t hz = HAL_RCC_GetPCLK1Freq();
    if ((RCC->CFGR & RCC_CFGR_PPRE1) != 0U)
    {
        hz *= APB_TIMER_CLOCK_MULTIPLIER;
    }
    else
    {
        /* No action. */
    }
    TIM2->PSC = ((hz / LID_TIMER_TICKS_PER_SECOND) - 1U);
    TIM2->ARR = LID_TIMER_PERIOD;
    TIM2->EGR = TIM_EGR_UG;
    TIM2->SR = 0U;
    expirations = 0U;
    remaining = 0U;
    expired = 0U;
    HAL_NVIC_SetPriority(TIM2_IRQn, APP_IRQ_PRIORITY, 0U);
    HAL_NVIC_EnableIRQ(TIM2_IRQn);
}

void LidTimer_Start(uint32_t seconds)
{
    uint32_t p = __get_PRIMASK();
    __disable_irq();
    TIM2->CR1 = 0U;
    TIM2->DIER = 0U;
    remaining = seconds;
    expired = 0U;
    TIM2->CNT = 0U;
    TIM2->EGR = TIM_EGR_UG;
    TIM2->SR = 0U;
    HAL_NVIC_ClearPendingIRQ(TIM2_IRQn);
    if (seconds != 0U)
    {
        TIM2->DIER = TIM_DIER_UIE;
        TIM2->CR1 = TIM_CR1_CEN;
    }
    else
    {
        /* No action. */
    }
    __set_PRIMASK(p);
}

void LidTimer_Stop(void)
{
    uint32_t p = __get_PRIMASK();
    __disable_irq();
    TIM2->CR1 = 0U;
    TIM2->DIER = 0U;
    TIM2->SR = 0U;
    remaining = 0U;
    expired = 0U;
    HAL_NVIC_ClearPendingIRQ(TIM2_IRQn);
    __set_PRIMASK(p);
}

void LidTimer_IRQHandler(void)
{
    if (((TIM2->SR & TIM_SR_UIF) != 0U) && ((TIM2->DIER & TIM_DIER_UIE) != 0U))
    {
        TIM2->SR = 0U;
        if ((remaining != 0U) && (--remaining == 0U))
        {
            TIM2->CR1 = 0U;
            TIM2->DIER = 0U;
            expired = 1U;
            expirations++;
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
}

uint8_t LidTimer_TakeExpired(void)
{
    uint32_t p = __get_PRIMASK();
    __disable_irq();
    uint8_t v = expired;
    expired = 0U;
    __set_PRIMASK(p);
    return v;
}

uint32_t LidTimer_Count(void)
{
    return expirations;
}
