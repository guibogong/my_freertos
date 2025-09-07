#include "portmacro.h"
#include "projdefs.h"
#include "task.h"




static uint32_t  uxCriticalNesting = 0xaaaaaaaa;
uint32_t xTickCount = 0;



static void prvTaskExitError( void )
{
    for(;;);
}

void prvStartFirstTask(void)
{
    //__asm("PRESERVE8");
    __asm("ldr r0, =0xE000ED08");
    __asm("ldr r0, [r0]");
    __asm("ldr r0, [r0]");
  
    __asm("msr msp, r0");
    
    __asm("cpsie i");
    __asm("cpsie f");
    __asm("dsb");
    __asm("isb");
  
    __asm("svc 0");
    __asm("nop");
    __asm("nop");
}



void vPortSVCHandler( void )
{
    extern TCB_T* pxCurrentTCB;
    //__asm("PRESERVE8");

    __asm("ldr r3, =pxCurrentTCB");
    __asm("ldr r1, [r3]");
    __asm("ldr r0, [r1]");
  
    __asm("ldmia r0!, {r4 - r11}");
    
    __asm("msr psp, r0");
    __asm("isb");
    __asm("mov r0, #0");
    __asm("msr basepri, r0");
    __asm("orr r14, #0xd");
  
    
    __asm("bx r14");
}


StackType_t *pxPortInitialiseStack( StackType_t *pxTopOfStack,
                                    TaskFunction_t pxCode,
                                     void *pvParameters )
{
    pxTopOfStack--;
    *pxTopOfStack = portINITIAL_XPSR;
    pxTopOfStack--;
    *pxTopOfStack = ((StackType_t) pxCode) & portSTART_ADDRESS_MASK;
    pxTopOfStack--;
    *pxTopOfStack = (StackType_t)prvTaskExitError;
    pxTopOfStack -= 5;
    *pxTopOfStack = (StackType_t)pvParameters;
  
    pxTopOfStack -= 8;
  
    return pxTopOfStack;
    
}


BaseType_t xPortStartScheduler( void )
{
    portNVIC_SYSPRI2_REG |= portNVIC_PENDSV_PRI;
    portNVIC_SYSPRI2_REG |= portNVIC_SYSTICK_PRI;
  
    vPortSetupTimeInterrupt();
  
    prvStartFirstTask();
  
    return 0;
}



void xPortPendSVHandler( void )
{
    extern TCB_T* pxCurrentTCB;
    extern void vTaskSwitchContext(void);

    //__asm("PRESERVE8");
    
    __asm("mrs r0, psp");
    __asm("isb");
  
    __asm("ldr r3, =pxCurrentTCB");
    __asm("ldr r2, [r3]");
  
    __asm("stmdb r0!, {r4-r11}");
    __asm("str r0, [r2]");
  
    __asm("stmdb sp!, {r3, r14}");
    __asm volatile(
                   "mov r0, %0"
                   : 
                   :"i"(configMAX_SYSCALL_INTERRUPT_PRIORITY)
                   :"r0"
 ); 
    __asm("msr basepri, r0");
    __asm("dsb");
    __asm("isb");

    __asm("bl vTaskSwitchContext");
    __asm("mov r0, #0"); 
    __asm("msr basepri, r0");
    __asm("ldmia sp!, {r3, r14}");
    
    __asm("ldr r1, [r3]");
    __asm("ldr r0, [r1]");
    __asm("ldmia r0!, {r4-r11}");
    __asm("msr psp, r0");
    __asm("isb");
    __asm("bx r14");
    __asm("nop");
}



void vPortEnterCritical( void )
{
    portDISABLE_INTERUPTS();
    uxCriticalNesting++;
    
    if( uxCriticalNesting == 1 )
    {
        configASSERT( (portNVIC_INT_CTRL_REG & portVECTACTIVE_MASK) == 0 );
      
    }
    
    
}



void vPortExitCritical( void )
{
    configASSERT( uxCriticalNesting );
    uxCriticalNesting--;
    if( uxCriticalNesting )
    {
        portENABLE_INTERRUPTS();
    }
}


void xPortSysTickHandler( void )
{
    vPortRaiseBASEPRI();
    xTaskIncrementTick();
    portENABLE_INTERRUPTS();
    
}


void vPortSetupTimeInterrupt( void )
{

    portNVIC_SYSTICK_LOAD_REG = ( configSYSTICK_CLOCK_HZ / configTICK_RATE_HZ ) - 1UL;
    
    portNVIC_SYSTICK_CTRL_REG = ( portNVIC_SYSTICK_CLK_BIT |
                                  portNVIC_SYSTICK_INT_BIT |
                                  portNVIC_SYSTICK_ENABLE_BIT
                                 );
}

uint32_t ulPortRaiseBASEPRI( void )
{
    uint32_t ulReturn, ulNewBASEPRI = configMAX_SYSCALL_INTERRUPT_PRIORITY;
    //__ASM( "mrs ulReturn, basepri" ); 
   __asm volatile (
        "mrs %0, basepri" 
        : "=r" (ulReturn)
        :        
        : "memory");
    __asm volatile (
        "msr basepri, %0"
        :
        : "r" (ulNewBASEPRI) 
        : "memory");
    //__ASM( "msr basepri, ulNewBASEPRI");
    __asm("dsb");
    __asm("isb"); 
    return ulReturn;    
}