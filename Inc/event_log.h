#ifndef EVENT_LOG_H
#define EVENT_LOG_H
#include <stdint.h>
#include <stddef.h>
void EventLog_Init(void);
void EventLog_Add(uint32_t now,const char *event);
void EventLog_Read(unsigned page,char *out,size_t size);
#endif
