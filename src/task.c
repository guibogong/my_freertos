#include "task.h"





/*
********************************************************
*                     全局变量
********************************************************
*/
List_t pxReadyTasksLists[ configMAX_PRIORITIES ];
StackType_t IdleTaskStack[configMINIMAL_STACK_SIZE]; 
static volatile UBaseType_t uxCurrentNumberOfTasks 	= ( UBaseType_t ) 0U;

TCB_T *pxCurrentTCB;
extern TCB_T Task1TCB;

extern TCB_T Task2TCB;

extern TCB_T IdleTaskTCB;
extern TaskHandle_t xIdleTaskHandle;
extern uint32_t xTickCount;


#define tskIDLE_PRIORITY ( ( UBaseType_t ) 0U )
static volatile UBaseType_t uxTopReadyPriority = tskIDLE_PRIORITY;


/* 将任务添加到就绪列表 */                                    
#define prvAddTaskToReadyList( pxTCB )																   \
	taskRECORD_READY_PRIORITY( ( pxTCB )->uxPriority );												   \
	vListInsertEnd( &( pxReadyTasksLists[ ( pxTCB )->uxPriority ] ), &( ( pxTCB )->xStateListItem ) ); \
  
static List_t xDelayedTaskList1;
static List_t xDelayedTaskList2;
static List_t * volatile pxDelayedTaskList;
static List_t * volatile pxOverflowDelayedTaskList;

static volatile TickType_t xNextTaskUnblockTime		= ( TickType_t ) 0U;
static volatile BaseType_t xNumOfOverflows 			= ( BaseType_t ) 0;



/* 查找最高优先级的就绪任务：通用方法 */                                    
#if ( configUSE_PORT_OPTIMISED_TASK_SELECTION == 0 )
	/* uxTopReadyPriority 存的是就绪任务的最高优先级 */
	#define taskRECORD_READY_PRIORITY( uxPriority )														\
	{																									\
		if( ( uxPriority ) > uxTopReadyPriority )														\
		{																								\
			uxTopReadyPriority = ( uxPriority );														\
		}																								\
	} /* taskRECORD_READY_PRIORITY */

	/*-----------------------------------------------------------*/

	#define taskSELECT_HIGHEST_PRIORITY_TASK()															\
	{																									\
	UBaseType_t uxTopPriority = uxTopReadyPriority;														\
																										\
		/* 寻找包含就绪任务的最高优先级的队列 */                                                          \
		while( listLIST_IS_EMPTY( &( pxReadyTasksLists[ uxTopPriority ] ) ) )							\
		{																								\
			--uxTopPriority;																			\
		}																								\
																										\
		/* 获取优先级最高的就绪任务的TCB，然后更新到pxCurrentTCB */							            \
		listGET_OWNER_OF_NEXT_ENTRY( pxCurrentTCB, &( pxReadyTasksLists[ uxTopPriority ] ) );			\
		/* 更新uxTopReadyPriority */                                                                    \
		uxTopReadyPriority = uxTopPriority;																\
	} /* taskSELECT_HIGHEST_PRIORITY_TASK */

	/*-----------------------------------------------------------*/

    /* 这两个宏定义只有在选择优化方法时才用，这里定义为空 */
	#define taskRESET_READY_PRIORITY( uxPriority )
	#define portRESET_READY_PRIORITY( uxPriority, uxTopReadyPriority )
    
/* 查找最高优先级的就绪任务：根据处理器架构优化后的方法 */
#else /* configUSE_PORT_OPTIMISED_TASK_SELECTION */

	#define taskRECORD_READY_PRIORITY( uxPriority )	portRECORD_READY_PRIORITY( uxPriority, uxTopReadyPriority )

	/*-----------------------------------------------------------*/

	#define taskSELECT_HIGHEST_PRIORITY_TASK()														    \
	{																								    \
	UBaseType_t uxTopPriority;																		    \
																									    \
		/* 寻找包含就绪任务的最高优先级的队列 */								                            \
		portGET_HIGHEST_PRIORITY( uxTopPriority, uxTopReadyPriority );								    \
		/* 获取优先级最高的就绪任务的TCB，然后更新到pxCurrentTCB */                                       \
		listGET_OWNER_OF_NEXT_ENTRY( pxCurrentTCB, &( pxReadyTasksLists[ uxTopPriority ] ) );		    \
	} /* taskSELECT_HIGHEST_PRIORITY_TASK() */

	/*-----------------------------------------------------------*/
#if 0
	#define taskRESET_READY_PRIORITY( uxPriority )														\
	{																									\
		if( listCURRENT_LIST_LENGTH( &( pxReadyTasksLists[ ( uxPriority ) ] ) ) == ( UBaseType_t ) 0 )	\
		{																								\
			portRESET_READY_PRIORITY( ( uxPriority ), ( uxTopReadyPriority ) );							\
		}																								\
	}
#else
    #define taskRESET_READY_PRIORITY( uxPriority )											    \
    {																							\
            portRESET_READY_PRIORITY( ( uxPriority ), ( uxTopReadyPriority ) );					\
    }
#endif
    
#endif /* configUSE_PORT_OPTIMISED_TASK_SELECTION */
    

    
static void prvResetNextTaskUnblockTime( void )
{
    TCB_T *pxTCB;

	if( listLIST_IS_EMPTY( pxDelayedTaskList ) != pdFALSE )
	{
		/* The new current delayed list is empty.  Set xNextTaskUnblockTime to
		the maximum possible value so it is	extremely unlikely that the
		if( xTickCount >= xNextTaskUnblockTime ) test will pass until
		there is an item in the delayed list. */
		xNextTaskUnblockTime = portMAX_DELAY;
	}
	else
	{
		/* The new current delayed list is not empty, get the value of
		the item at the head of the delayed list.  This is the time at
		which the task at the head of the delayed list should be removed
		from the Blocked state. */
		( pxTCB ) = ( TCB_T * ) listGET_OWNER_OF_HEAD_ENTRY( pxDelayedTaskList );
		xNextTaskUnblockTime = listGET_LIST_ITEM_VALUE( &( ( pxTCB )->xStateListItem ) );
	}
}

    
/* 
 * 当系统时基计数器溢出的时候，延时列表pxDelayedTaskList 和
 * pxOverflowDelayedTaskList要互相切换
 */
#define taskSWITCH_DELAYED_LISTS()\
{
	List_t *pxTemp;
	pxTemp = pxDelayedTaskList;
	pxDelayedTaskList = pxOverflowDelayedTaskList;
	pxOverflowDelayedTaskList = pxTemp;
	xNumOfOverflows++;
	prvResetNextTaskUnblockTime();
}


void prvInitialiseTaskLists( void )
{
    UBaseType_t uxPriority;
    for( uxPriority = ( UBaseType_t )0;
         uxPriority < ( UBaseType_t ) configMAX_PRIORITIES;
         uxPriority++  )
    {
        vListInitialise( &( pxReadyTasksLists[ uxPriority ] ) );
    }
    vListInitialise( &xDelayedTaskList1 );
	  vListInitialise( &xDelayedTaskList2 );
    
    pxDelayedTaskList = &xDelayedTaskList1;
	  pxOverflowDelayedTaskList = &xDelayedTaskList2;
    
    
}

static portTASK_FUNCTION( prvIdleTask, pvParameters );


static void prvInitialiseNewTask( TaskFunction_t pxTaskCode,
                                const char * const pcName,
                                const uint32_t ulStackDepth,
                                void * const pvParameters,
                                UBaseType_t uxPriority,
                                TaskHandle_t * const pxCreatedTask,
                                TCB_T * const pxNewTCB)
{
    StackType_t *pxTopOfStack;
    UBaseType_t x;
  
    pxTopOfStack = pxNewTCB->pxStack + ( ulStackDepth - (uint32_t)1 );
  
    pxTopOfStack = (StackType_t *)( (uint32_t)pxTopOfStack & (~((uint32_t) 0x7)));
     
    for( x = ( UBaseType_t ) 0; x < ( UBaseType_t )configMAX_TASK_NAME_LEN; x++)
    {
        pxNewTCB->pcTaskName[x] =  pcName[x];
        if( pcName[x] == 0x00 )
        {
            break;
        }
    }
    
    pxNewTCB->pcTaskName[configMAX_TASK_NAME_LEN - 1] = '\0';
    
    vListInitialistItem( &( pxNewTCB->xStateListItem));
    listSET_LIST_ITEM_OWNER( &( pxNewTCB->xStateListItem ), pxNewTCB );
    
    if( uxPriority >= ( UBaseType_t ) configMAX_PRIORITIES )
    {
      uxPriority = ( UBaseType_t ) configMAX_PRIORITIES - ( UBaseType_t ) 1U;
    }
    pxNewTCB->uxPriority = uxPriority;
    
    pxNewTCB->pxTopofStack = pxPortInitialiseStack( pxTopOfStack,
                                                    pxTaskCode,
                                                    pvParameters);    

    if( ( void *) pxCreatedTask != NULL)
    {
        *pxCreatedTask = ( TaskHandle_t ) pxNewTCB;
    }
}



static void prvAddNewTaskToReadyList( TCB_T *pxNewTCB )
{
	/* 进入临界段 */
	taskENTER_CRITICAL();
	{
		uxCurrentNumberOfTasks++;
        
        /* 如果pxCurrentTCB为空，则将pxCurrentTCB指向新创建的任务 */
		if( pxCurrentTCB == NULL )
		{
			pxCurrentTCB = pxNewTCB;

			/* 如果是第一次创建任务，则需要初始化任务相关的列表 */
            if( uxCurrentNumberOfTasks == ( UBaseType_t ) 1 )
			{
				/* 初始化任务相关的列表 */
                prvInitialiseTaskLists();
			}
		}
		else /* 如果pxCurrentTCB不为空，则根据任务的优先级将pxCurrentTCB指向最高优先级任务的TCB */
		{
				if( pxCurrentTCB->uxPriority <= pxNewTCB->uxPriority )
				{
					pxCurrentTCB = pxNewTCB;
				}
		}
        
		/* 将任务添加到就绪列表 */
        prvAddTaskToReadyList( pxNewTCB );

	}
	/* 退出临界段 */
	taskEXIT_CRITICAL();
}

void vTaskStartScheduler( void )
{
    /*===================创建空闲任务start==============*/
    TCB_T *pxIdleTaskTCBBuffer = NULL;
    StackType_t *pxIdleTaskStackBuffer = NULL;
    uint32_t ulIdleTaskStackSize;
  
    
    vApplicationGetIdleTaskMemory( &pxIdleTaskTCBBuffer,
                                   &pxIdleTaskStackBuffer,
                                   &ulIdleTaskStackSize);
    xIdleTaskHandle = 
                  xTaskCreateStatic( (TaskFunction_t)prvIdleTask,
                  (char *)"IDLE",
                  (uint32_t)ulIdleTaskStackSize,
                  (void *)NULL,
                  (UBaseType_t) tskIDLE_PRIORITY,
                  (StackType_t *)pxIdleTaskStackBuffer,
                  (TCB_T *)pxIdleTaskTCBBuffer);
    
     xNextTaskUnblockTime = portMAX_DELAY;
     xTickCount = ( TickType_t ) 0U;
    
    if( xPortStartScheduler() != pdFALSE )
    {
    }
}

#if 1

TaskHandle_t xTaskCreateStatic( TaskFunction_t pxTaskCode,
                                const char * const pcName,
                                const uint32_t ulStackDepth,
                                void * const pvParameters,
                                UBaseType_t  uxPriority,
                                StackType_t* const puxStackBuffer,
                                TCB_T * const pxTaskBuffer)
{

    TCB_T *pxNewTCB;
    TaskHandle_t xRETURN;
  
    if( ( NULL !=  pxTaskBuffer ) && ( NULL != puxStackBuffer ) )
    {
        pxNewTCB = ( TCB_T * )pxTaskBuffer;
        pxNewTCB->pxStack = ( StackType_t * )puxStackBuffer;
        prvInitialiseNewTask(pxTaskCode,
                             pcName,
                             ulStackDepth,
                             pvParameters,
                             uxPriority,
                             &xRETURN,      
                             pxNewTCB);
      prvAddNewTaskToReadyList( pxNewTCB );
      
        
    }
    else
    {
        xRETURN = NULL;
    }
    
    return xRETURN;
}


static portTASK_FUNCTION( prvIdleTask, pvParameters )
{
	/* 防止编译器的警告 */
	( void ) pvParameters;
    
    for(;;)
    {
        /* 空闲任务暂时什么都不做 */
    }
}

#if 1
void vTaskSwitchContext( void )
{
   taskSELECT_HIGHEST_PRIORITY_TASK(); 
}

#else
void vTaskSwitchContext( void )
{
    if( &IdleTaskTCB == pxCurrentTCB )
    {
        if( 0 == Task1TCB.xTicksToDelay )
        {
            pxCurrentTCB  = &Task1TCB;
        }
        else if( 0 == Task2TCB.xTicksToDelay )
        {
            pxCurrentTCB  = &Task2TCB;
        }
        else
        {
            return;
        }
    }
    else
    {
        if( &Task1TCB == pxCurrentTCB )
        {
            if ( 0 == Task2TCB.xTicksToDelay )
            {
                pxCurrentTCB = &Task2TCB;
            }
            else if ( 0 != pxCurrentTCB->xTicksToDelay )
            {
                pxCurrentTCB = &IdleTaskTCB;
            }
            else
            {
                return;
            }
        }
        if( &Task2TCB == pxCurrentTCB )
        {
            if ( 0 == Task1TCB.xTicksToDelay )
            {
                pxCurrentTCB = &Task1TCB;
            }
            else if ( 0 != pxCurrentTCB->xTicksToDelay )
            {
                pxCurrentTCB = &IdleTaskTCB;
            }
            else
            {
                return;
            }
        }
    }
}


#endif


static void prvAddCurrentTaskToDelayedList( TickType_t xTicksToWait )
{
    TickType_t xTimeToWake;
    
    /* 获取系统时基计数器xTickCount的值 */
    const TickType_t xConstTickCount = xTickCount;

    /* 将任务从就绪列表中移除 */
	if( vListDelet( &( pxCurrentTCB->xStateListItem ) ) == ( UBaseType_t ) 0 )
	{
		/* 将任务在优先级位图中对应的位清除 */
        portRESET_READY_PRIORITY( pxCurrentTCB->uxPriority, uxTopReadyPriority );
	}

    /* 计算延时到期时，系统时基计数器xTickCount的值是多少 */
    xTimeToWake = xConstTickCount + xTicksToWait;

    /* 将延时到期的值设置为节点的排序值 */
    listSET_LIST_ITEM_VALUE( &( pxCurrentTCB->xStateListItem ), xTimeToWake );

    /* 溢出 */
    if( xTimeToWake < xConstTickCount )
    {
        vListInsert( pxOverflowDelayedTaskList, &( pxCurrentTCB->xStateListItem ) );
    }
    else /* 没有溢出 */
    {

        vListInsert( pxDelayedTaskList, &( pxCurrentTCB->xStateListItem ) );

        /* 更新下一个任务解锁时刻变量xNextTaskUnblockTime的值 */
        if( xTimeToWake < xNextTaskUnblockTime )
        {
            xNextTaskUnblockTime = xTimeToWake;
        }
    }	
}

void delay(uint32_t count)
{
    for(; count != 0; count--);
}

void vApplicationGetIdleTaskMemory( TCB_T **ppxIdleTaskTCBBuffer,
                                    StackType_t **ppxIdleTaskStackBuffer,
                                    uint32_t *pulIdleTaskStacksize)
{
    *ppxIdleTaskTCBBuffer = &IdleTaskTCB;
    *ppxIdleTaskStackBuffer = IdleTaskStack;
    *pulIdleTaskStacksize = configMINIMAL_STACK_SIZE;
}

void vTaskDelay( const TickType_t xTicksToDelay )
{
    TCB_T *pxTCB = NULL;
    
    pxTCB = pxCurrentTCB;
    
    /*pxTCB->xTicksToDelay = xTicksToDelay;
    
    taskRESET_READY_PRIORITY( pxTCB->uxPriority );*/
    prvAddCurrentTaskToDelayedList( xTicksToDelay );
    
    
    taskYIELD();
}







#if 0
void xTaskIncrementTick( void )
{
    TCB_t *pxTCB = NULL;
    BaseType_t i = 0;
    
    const TickType_t xConstTickCount = xTickCount + 1;
    xTickCount = xConstTickCount;
    
    /* 扫描就绪列表中所有线程的remaining_tick，如果不为0，则减1 */
	for(i=0; i<configMAX_PRIORITIES; i++)
	{
        pxTCB = ( TCB_t * ) listGET_OWNER_OF_HEAD_ENTRY( ( &pxReadyTasksLists[i] ) );
		if(pxTCB->xTicksToDelay > 0)
		{
			pxTCB->xTicksToDelay --;
            
            /* 延时时间到，将任务就绪 */
            if( pxTCB->xTicksToDelay ==0 )
            {
                //vListInsertEnd( &( pxReadyTasksLists[i] ), &( ((TCB_t *)(&Task1TCB))->xStateListItem ) );
                taskRECORD_READY_PRIORITY( pxTCB->uxPriority );
            }
		}
	}
    
    /* 任务切换 */
    portYIELD();
}

#else
void xTaskIncrementTick( void )
{
	TCB_T * pxTCB;
	TickType_t xItemValue;

	const TickType_t xConstTickCount = xTickCount + 1;
	xTickCount = xConstTickCount;

	/* 如果xConstTickCount溢出，则切换延时列表 */
	if( xConstTickCount == ( TickType_t ) 0U )
	{
		taskSWITCH_DELAYED_LISTS();
	}

	/* 最近的延时任务延时到期 */
	if( xConstTickCount >= xNextTaskUnblockTime )
	{
		for( ;; )
		{
			if( listLIST_IS_EMPTY( pxDelayedTaskList ) != pdFALSE )
			{
				/* 延时列表为空，设置xNextTaskUnblockTime为可能的最大值 */
				xNextTaskUnblockTime = portMAX_DELAY;
				break;
			}
			else /* 延时列表不为空 */
			{
				pxTCB = ( TCB_T * ) listGET_OWNER_OF_HEAD_ENTRY( pxDelayedTaskList );
				xItemValue = listGET_LIST_ITEM_VALUE( &( pxTCB->xStateListItem ) );

				/* 直到将延时列表中所有延时到期的任务移除才跳出for循环 */
                if( xConstTickCount < xItemValue )
				{
					xNextTaskUnblockTime = xItemValue;
					break;
				}

				/* 将任务从延时列表移除，消除等待状态 */
				( void ) vListDelet( &( pxTCB->xStateListItem ) );

				/* 将解除等待的任务添加到就绪列表 */
				prvAddTaskToReadyList( pxTCB );
			}
		}
	}/* xConstTickCount >= xNextTaskUnblockTime */
    
    /* 任务切换 */
    portYIELD();
}
#endif


#else






#endif