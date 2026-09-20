#ifndef CONSOLE_COMMANDS_H
#define CONSOLE_COMMANDS_H
#include "app_state.h"
#include <stddef.h>
void Console_Command(App *a,const char *line,uint32_t now,char *out,size_t size);
#endif
