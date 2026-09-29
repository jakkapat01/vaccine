#include "main.h"
#include "warning_leds.h"
/* Training Shield mapping from Exam2 BSP: four active-high LEDs. */
#define LED_A (GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7)
#define LED_B GPIO_PIN_6
static uint8_t active,on;
static uint32_t changed_at;
static void output(uint8_t value) {
    HAL_GPIO_WritePin(GPIOA,LED_A,value ? GPIO_PIN_SET:GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOB,LED_B,value ? GPIO_PIN_SET:GPIO_PIN_RESET);
}
void WarningLEDs_Init(void) {
    __HAL_RCC_GPIOA_CLK_ENABLE();__HAL_RCC_GPIOB_CLK_ENABLE();
    active=on=0;changed_at=0;output(0);
    GPIO_InitTypeDef g={0};g.Mode=GPIO_MODE_OUTPUT_PP;g.Pull=GPIO_NOPULL;g.Speed=GPIO_SPEED_FREQ_LOW;
    g.Pin=LED_A;HAL_GPIO_Init(GPIOA,&g);g.Pin=LED_B;HAL_GPIO_Init(GPIOB,&g);
}
void WarningLEDs_Process(uint8_t warning,uint32_t now) {
    if(!warning) {if(active) output(0);active=on=0;return;}
    if(!active) {active=on=1;changed_at=now;output(1);return;}
    if((uint32_t)(now-changed_at)>=250U) {changed_at=now;on=!on;output(on);}
}
