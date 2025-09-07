#ifndef _PORT_H
#define _PORT_H

#include "list.h"
#include "task.h"
#include "portmacro.h"
#include "projdefs.h"



#define portINITIAL_XPSR ( 0x01000000 )
#define portSTART_ADDRESS_MASK ( ( StackType_t ) 0xfffffffeUL )

#define portNVIC_SYSPRI2_REG (*(( volatile uint32_t *) 0xe000ed20))

#define portNVIC_PENDSV_PRI (((uint32_t) configKERNEL_INTERRUPT_PRIORITY ) << 16UL)
#define portNVIC_SYSTICK_PRI (((uint32_t) configKERNEL_INTERRUPT_PRIORITY ) << 24UL )
#define portVECTACTIVE_MASK   0xffffffffUL
#define xPortPendSVHandler PendSV_Handler
#define xPortSysTickHandler SysTick_Handler
#define vPortSVCHandler SVC_Handler

#define portNVIC_SYSTICK_CTRL_REG (*((volatile uint32_t *) 0xe000e010 ))
#define portNVIC_SYSTICK_LOAD_REG (*((volatile uint32_t *) 0xe000e014 ))
#ifndef configSYSTICK_CLOCK_HZ
    #define configSYSTICK_CLOCK_HZ configCPU_CLOCK_HZ
    #define portNVIC_SYSTICK_CLK_BIT ( 1UL << 2UL )
#else
    #define portNVIC_SYSTICK_CLK_BIT ( 0 )
#endif

#define portNVIC_SYSTICK_INT_BIT ( 1UL << 1UL )
#define portNVIC_SYSTICK_ENABLE_BIT ( 1UL << 0UL )


StackType_t *pxPortInitialiseStack( StackType_t *pxTopOfStack,
                                    TaskFunction_t pxCode,
                                     void *pvParameters );;
                                     
void prvStartFirstTask(void);

void vPortSVCHandler( void );

BaseType_t xPortStartScheduler( void );

void vPortSetupTimeInterrupt( void );

void vPortEnterCritical( void );

void vPortExitCritical( void );




#endif