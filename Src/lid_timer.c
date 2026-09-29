#include "main.h"
#include "lid_timer.h"
static volatile uint32_t remaining,expirations;
static volatile uint8_t expired;
void LidTimer_Init(void) {
    __HAL_RCC_TIM2_CLK_ENABLE();
    TIM2->CR1=0;TIM2->DIER=0;
    uint32_t hz=HAL_RCC_GetPCLK1Freq();
    if((RCC->CFGR & RCC_CFGR_PPRE1)!=0) hz*=2U;
    TIM2->PSC=hz/10000U-1U;TIM2->ARR=9999U;
    TIM2->EGR=TIM_EGR_UG;TIM2->SR=0;
    remaining=expirations=0;expired=0;
    HAL_NVIC_SetPriority(TIM2_IRQn,5,0);HAL_NVIC_EnableIRQ(TIM2_IRQn);
}
void LidTimer_Start(uint32_t seconds) {
    uint32_t p=__get_PRIMASK();__disable_irq();
    TIM2->CR1=0;TIM2->DIER=0;remaining=seconds;expired=0;
    TIM2->CNT=0;TIM2->EGR=TIM_EGR_UG;TIM2->SR=0;
    HAL_NVIC_ClearPendingIRQ(TIM2_IRQn);
    if(seconds) {TIM2->DIER=TIM_DIER_UIE;TIM2->CR1=TIM_CR1_CEN;}
    __set_PRIMASK(p);
}
void LidTimer_Stop(void) {
    uint32_t p=__get_PRIMASK();__disable_irq();
    TIM2->CR1=0;TIM2->DIER=0;TIM2->SR=0;remaining=0;expired=0;
    HAL_NVIC_ClearPendingIRQ(TIM2_IRQn);__set_PRIMASK(p);
}
void LidTimer_IRQHandler(void) {
    if((TIM2->SR&TIM_SR_UIF) && (TIM2->DIER&TIM_DIER_UIE)) {
        TIM2->SR=0;
        if(remaining && --remaining==0) {
            TIM2->CR1=0;TIM2->DIER=0;expired=1;expirations++;
        }
    }
}
uint8_t LidTimer_TakeExpired(void) {
    uint32_t p=__get_PRIMASK();__disable_irq();
    uint8_t v=expired;expired=0;__set_PRIMASK(p);return v;
}
uint32_t LidTimer_Count(void) {return expirations;}
