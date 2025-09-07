#ifndef _H_FREERTOS_H
#define _H_FREERTOS_H

#include "portmacro.h"
#include "list.h"

typedef struct tskTaskControlBlock
{
    volatile StackType_t        *pxTopofStack;
    ListItem_t                   xStateListItem;
    StackType_t                  *pxStack;
    char                         pcTaskName[16];
    TickType_t                   xTicksToDelay;
    UBaseType_t                  uxPriority;
}tskTCB;

typedef tskTCB  TCB_T;
#endif