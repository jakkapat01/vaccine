#include "main.h"
#include "buttons.h"
#include "app_state.h"
/* All four EXTI IRQs share priority 5: one producer, main is the consumer.
 * Completed presses are queued on release, never execute application code here. */
static GPIO_TypeDef *const ports[4]={GPIOA,GPIOB,GPIOB,GPIOB};
static const uint16_t pins[4]={GPIO_PIN_10,GPIO_PIN_3,GPIO_PIN_5,GPIO_PIN_4};
static const IRQn_Type irqs[4]={EXTI15_10_IRQn,EXTI3_IRQn,EXTI9_5_IRQn,EXTI4_IRQn};
static volatile uint32_t irq_count[4],dropped;
static uint32_t pressed_at[4],accepted_at[4];
static uint8_t down,blocked,ready;
static volatile uint8_t queue[16],head,tail;
void Buttons_Init(void) {
    GPIO_InitTypeDef g={0};ready=0;
    __HAL_RCC_GPIOA_CLK_ENABLE();__HAL_RCC_GPIOB_CLK_ENABLE();__HAL_RCC_SYSCFG_CLK_ENABLE();
    for(unsigned i=0;i<4;i++) HAL_NVIC_DisableIRQ(irqs[i]);
    g.Pull=GPIO_PULLUP;g.Mode=GPIO_MODE_IT_RISING_FALLING;
    g.Pin=GPIO_PIN_10;HAL_GPIO_Init(GPIOA,&g);
    g.Pin=GPIO_PIN_3|GPIO_PIN_4|GPIO_PIN_5;HAL_GPIO_Init(GPIOB,&g);
    down=0;head=tail=0;dropped=0;
    for(unsigned i=0;i<4;i++) {
        irq_count[i]=0;pressed_at[i]=HAL_GetTick();accepted_at[i]=pressed_at[i]-40U;
        if(HAL_GPIO_ReadPin(ports[i],pins[i])==GPIO_PIN_RESET) down|=(1U<<i);
        __HAL_GPIO_EXTI_CLEAR_IT(pins[i]);
        HAL_NVIC_ClearPendingIRQ(irqs[i]);HAL_NVIC_SetPriority(irqs[i],5,0);
    }
    /* A key held during boot must be released before it can act. */
    blocked=down;ready=1;
    for(unsigned i=0;i<4;i++) HAL_NVIC_EnableIRQ(irqs[i]);
}
void Buttons_Edge(uint16_t pin) {
    if(!ready) return;
    for(unsigned i=0;i<4;i++) if(pin==pins[i]) {
        irq_count[i]++;
        uint32_t now=HAL_GetTick();uint8_t bit=(uint8_t)(1U<<i);
        if(HAL_GPIO_ReadPin(ports[i],pin)==GPIO_PIN_RESET) {
            if(!(down&bit)) pressed_at[i]=now;
            down|=bit;
            if(down&(down-1U)) blocked|=down; /* Suppress overlapping keys. */
        } else {
            if((down&bit) && !(blocked&bit) && (uint32_t)(now-pressed_at[i])>=40U && (uint32_t)(now-accepted_at[i])>=40U) {
                uint8_t next=(head+1U)&15U;
                if(next==tail) dropped++;
                else {queue[head]=bit;__DMB();head=next;}
                accepted_at[i]=now;
            }
            down&=(uint8_t)~bit;blocked&=(uint8_t)~bit;
        }
        return;
    }
}
uint8_t Buttons_GetEvent(void) {
    if(tail==head) return 0;
    __DMB();uint8_t event=queue[tail];__DMB();tail=(tail+1U)&15U;return event;
}
void Buttons_GetIrqStats(uint32_t counts[4],uint32_t *lost) {
    uint32_t p=__get_PRIMASK();__disable_irq();
    for(unsigned i=0;i<4;i++) counts[i]=irq_count[i];
    *lost=dropped;__set_PRIMASK(p);
}
