#include "event_log.h"
#include <stdio.h>
#include <string.h>
static struct {uint32_t ms,seq;char text[64];} rows[16];
static unsigned head,count;
static uint32_t sequence;
void EventLog_Init(void) {head=count=0;sequence=0;}
void EventLog_Add(uint32_t now,const char *event) {
    rows[head].ms=now;rows[head].seq=++sequence;
    snprintf(rows[head].text,sizeof(rows[head].text),"%s",event);
    head=(head+1U)%16U;if(count<16) count++;
}
void EventLog_Read(unsigned page,char *out,size_t size) {
    if(!size) return;
    snprintf(out,size,"LOG RAM page %u/3 newest first; resets on reboot\r\n",page);
    for(unsigned j=page*4U;j<count && j<page*4U+4U;j++) {
        unsigned i=(head+15U-j)%16U;size_t used=strlen(out);
        if(used>=size-1) break;
        snprintf(out+used,size-used,"#%lu %lums %s\r\n",(unsigned long)rows[i].seq,(unsigned long)rows[i].ms,rows[i].text);
    }
}
