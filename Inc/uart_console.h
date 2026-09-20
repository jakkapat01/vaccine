#ifndef UART_CONSOLE_H
#define UART_CONSOLE_H
#include "app_state.h"
void UARTConsole_Init(void);
void UARTConsole_Process(App *app);
void UARTConsole_Log(const char *line);
void UARTConsole_IRQHandler(void);
#endif
