#ifndef __TASK_H
#define __TASK_H

#include "FreeRTOS.h"
#include "projdefs.h"
#include "task.h"
#include "FreeRTOSConfig.h"
#include "port.h"
#include "assert.h"
#include "list.h"
#include "portmacro.h"
#define taskYIELD()            portYIELD()

#define taskENTER_CRITICAL() portENTER_CRITICAL()
#define taskENTER_CRITICAL_FROM_ISR() portSET_INTERRUPT_MASK_FROM_ISR()
#define taskEXIT_CRITICAL() portEXIT_CRITICAL()
#define configASSERT(x)      assert(x)
#define taskEXIT_CRITICAL_FROM_ISR( x ) portCLEAR_INTERRUPT_MASK_FROM_ISR( x )



typedef void * TaskHandle_t;
void prvInitialiseTaskLists( void );

TaskHandle_t xTaskCreateStatic( TaskFunction_t pxTaskCode,
                                const char * const pcName,
                                const uint32_t ulStackDepth,
                                void * const pvParameters,
                                UBaseType_t  uxPriority,
                                StackType_t* const puxStackBuffer,
                                TCB_T * const pxTaskBuffer);

void vTaskStartScheduler( void );  
void delay(uint32_t count); 

void vApplicationGetIdleTaskMemory( TCB_T **ppxIdleTaskTCBBuffer, 
                                    StackType_t **ppxIdleTaskStackBuffer,
                                    uint32_t *pulIdleTaskStackSize );                                
 
void  xTaskIncrementTick( void );                             
                                
void vTaskDelay( const TickType_t xTicksToDelay );                                
                                
#endif