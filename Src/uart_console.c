#include "main.h"
#include "uart_console.h"
#include "console_commands.h"
#include <string.h>
#include <stdio.h>
#include "buttons.h"
/* USART2 PA2/PA3 to ST-LINK virtual COM. 115200 8N1. */
static volatile uint8_t rx[256];
static volatile uint16_t rh,rt;
static volatile uint8_t rx_overflow;
static char tx[2048];static uint16_t th,tt;
static char line[96];static unsigned used;static uint8_t dropping;
void UARTConsole_Init(void) {
    GPIO_InitTypeDef g={0};__HAL_RCC_GPIOA_CLK_ENABLE();__HAL_RCC_USART2_CLK_ENABLE();
    g.Pin=GPIO_PIN_2|GPIO_PIN_3;g.Mode=GPIO_MODE_AF_PP;g.Pull=GPIO_PULLUP;
    g.Speed=GPIO_SPEED_FREQ_VERY_HIGH;g.Alternate=GPIO_AF7_USART2;HAL_GPIO_Init(GPIOA,&g);
    USART2->CR1=0;USART2->CR2=0;USART2->CR3=0;
    USART2->BRR=(HAL_RCC_GetPCLK1Freq()+57600U)/115200U;
    USART2->CR1=USART_CR1_UE|USART_CR1_TE|USART_CR1_RE|USART_CR1_RXNEIE;
    HAL_NVIC_SetPriority(USART2_IRQn,6,0);HAL_NVIC_EnableIRQ(USART2_IRQn);
    UARTConsole_Log("VACCINE CLASSROOM MODEL - RAM RESET\r\nIN REAL SENSOR / OUT SIM CONSOLE / LID SIM D2\r\n115200 8N1; HELP for commands\r\n");
}
void UARTConsole_IRQHandler(void) {
    uint32_t sr=USART2->SR;
    if(sr&(USART_SR_RXNE|USART_SR_ORE|USART_SR_NE|USART_SR_FE|USART_SR_PE)) {
        uint8_t c=(uint8_t)USART2->DR;
        if(sr&(USART_SR_ORE|USART_SR_NE|USART_SR_FE|USART_SR_PE)) {rx_overflow=1;return;}
        uint16_t next=(rh+1U)&255U;
        if(next==rt) rx_overflow=1;
        else {rx[rh]=c;rh=next;}
    }
}
void UARTConsole_Log(const char *s) {
    unsigned length=(unsigned)strlen(s),space=(tt-th-1U)&2047U;
    if(length>space) return; /* Drop whole diagnostic, never partial reply. */
    while(*s) {tx[th]=*s++;th=(th+1U)&2047U;}
}
void UARTConsole_Process(App *a) {
    if(rx_overflow) {uint32_t p=__get_PRIMASK();__disable_irq();rt=rh;rx_overflow=0;__set_PRIMASK(p);dropping=1;used=0;}
    unsigned budget=64;
    while(rt!=rh && budget--) {
        /* Reserve room for an entire command reply before executing a command. */
        if(((tt-th-1U)&2047U)<512U) break;
        char c=(char)rx[rt];rt=(rt+1U)&255U;
        if(c=='\r'||c=='\n') {
            if(dropping) {UARTConsole_Log("ERROR LINE_OVERFLOW\r\n");dropping=0;used=0;}
            else if(used) {char reply[512];line[used]=0;if(!strcmp(line,"IRQ")) {
                    uint32_t counts[4],lost;Buttons_GetIrqStats(counts,&lost);
                    snprintf(reply,sizeof(reply),"IRQ EDGES D2=%lu D3=%lu D4=%lu D5=%lu DROPPED=%lu\r\n",
                        (unsigned long)counts[0],(unsigned long)counts[1],(unsigned long)counts[2],(unsigned long)counts[3],(unsigned long)lost);
                } else Console_Command(a,line,HAL_GetTick(),reply,sizeof(reply));UARTConsole_Log(reply);used=0;}
        } else if(!dropping) {
            if(c=='\b'||c==127) {if(used) used--;}
            else if(c=='\t'||(c>=32 && c<=126)) {
                if(used<sizeof(line)-1) line[used++]=c;else {dropping=1;used=0;}
            } else {dropping=1;used=0;}
        }
    }
    budget=64;
    while(tt!=th && budget-- && (USART2->SR&USART_SR_TXE)) {USART2->DR=(uint8_t)tx[tt];tt=(tt+1U)&2047U;}
}
