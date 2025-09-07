#ifndef PORTMACRO_H
#define PORTMACRO_H

#include "stdint.h"
#include "stddef.h" 
#include "arm_acle.h"
#include "cmsis_compiler.h"
#include "FreeRTOSConfig.h"


/* 数据类型重定义 */
#define portCHAR		char
#define portFLOAT		float
#define portDOUBLE		double
#define portLONG		long
#define portSHORT		short
#define portSTACK_TYPE	uint32_t
#define portBASE_TYPE	long

typedef portSTACK_TYPE StackType_t;
typedef long BaseType_t;
typedef unsigned long UBaseType_t;

#if( configUSE_16_BIT_TICKS == 1 )
	typedef uint16_t TickType_t;
	#define portMAX_DELAY ( TickType_t ) 0xffff
#else
	typedef uint32_t TickType_t;
	#define portMAX_DELAY ( TickType_t ) 0xffffffffUL

#endif

/* 中断控制状态寄存器：0xe000ed04
 * Bit 28 PENDSVSET: PendSV 悬起位
 */
#define portNVIC_INT_CTRL_REG		( * ( ( volatile uint32_t * ) 0xe000ed04 ) )
#define portNVIC_PENDSVSET_BIT		( 1UL << 28UL )

#define portSY_FULL_READ_WRITE		( 15 )
#define portENTER_CRITICAL() vPortEnterCritical()
#define portDISABLE_INTERRUPTS() vPortRaiseBASEPRI()
#define portEXIT_CRITICAL() vPortExitCritical()
#define portENABLE_INTERRUPTS() vPortSetBASEPRI( 0 )
#define portCLEAR_INTERRUPT_MASK_FROM_ISR(x) vPortSetBASEPRI(x)
#define portTASK_FUNCTION( vFunction, pvParameters ) void vFunction( void *pvParameters )
#define configUSE_PORT_OPTIMISED_TASK_SELECTION 1


#ifndef portFORCE_INLINE
	#define portFORCE_INLINE inline
#endif
  
  
#ifndef configUSE_PORT_OPTIMISED_TASK_SELECTION
	#define configUSE_PORT_OPTIMISED_TASK_SELECTION 1
#endif

#if configUSE_PORT_OPTIMISED_TASK_SELECTION == 1

	/* 检测优先级配置 */
	#if( configMAX_PRIORITIES > 32 )
		#error configUSE_PORT_OPTIMISED_TASK_SELECTION can only be set to 1 when configMAX_PRIORITIES is less than or equal to 32.  It is very rare that a system requires more than 10 to 15 difference priorities as tasks that share a priority will time slice.
	#endif

	/* 根据优先级设置/清除优先级位图中相应的位 */
	#define portRECORD_READY_PRIORITY( uxPriority, uxReadyPriorities ) ( uxReadyPriorities ) |= ( 1UL << ( uxPriority ) )
	#define portRESET_READY_PRIORITY( uxPriority, uxReadyPriorities ) ( uxReadyPriorities ) &= ~( 1UL << ( uxPriority ) )

	/*-----------------------------------------------------------*/

	#define portGET_HIGHEST_PRIORITY( uxTopPriority, uxReadyPriorities ) uxTopPriority = ( 31UL - ( uint32_t ) __clz( ( uxReadyPriorities ) ) )

#endif /* taskRECORD_READY_PRIORITY */
        

#define portYIELD()																\
{																				\
	/* 触发PendSV，产生上下文切换 */								                \
	portNVIC_INT_CTRL_REG = portNVIC_PENDSVSET_BIT;								\
	__DSB(  );											\
	__ISB(  );											\
}


#define portDISABLE_INTERUPTS() vPortRaiseBASEPRI()
static portFORCE_INLINE void  vPortRaiseBASEPRI( void )
{
    uint32_t ulNewBASEPRI = configMAX_SYSCALL_INTERRUPT_PRIORITY;
    __asm volatile ( "msr basepri, %0" :: "r" (ulNewBASEPRI) : "memory" );
    __asm("dsb");
    __asm("isb");
}

#define portSET_INTERRUPT_MASK_FROM_ISR() ulPortRaiseBASEPRI()


#define portENABLE_INTERRUPTS() vPortSetBASEPRI( 0 )
#define portCLEAR_INTERRUPT_MASK_FROM_ISR(x) vPortSetBASEPRI(x)
static portFORCE_INLINE void vPortSetBASEPRI( uint32_t ulBASEPRI )
{
    //__ASM( "msr basepri, ulBASEPRI" );
    __asm volatile (
        "msr basepri, %0"
        :
        : "r" (ulBASEPRI) 
        : "memory");
}

uint32_t ulPortRaiseBASEPRI( void );







#endif /* PORTMACRO_H */
