#include "task.h"





/*
********************************************************
*                     ȫ�ֱ���
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


/* ���������ӵ������б� */                                    
#define prvAddTaskToReadyList( pxTCB )																   \
	taskRECORD_READY_PRIORITY( ( pxTCB )->uxPriority );												   \
	vListInsertEnd( &( pxReadyTasksLists[ ( pxTCB )->uxPriority ] ), &( ( pxTCB )->xStateListItem ) ); \
  
static List_t xDelayedTaskList1;
static List_t xDelayedTaskList2;
static List_t * volatile pxDelayedTaskList;
static List_t * volatile pxOverflowDelayedTaskList;

static volatile TickType_t xNextTaskUnblockTime		= ( TickType_t ) 0U;
static volatile BaseType_t xNumOfOverflows 			= ( BaseType_t ) 0;



/* ����������ȼ��ľ�������ͨ�÷��� */                                    
#if ( configUSE_PORT_OPTIMISED_TASK_SELECTION == 0 )
	/* uxTopReadyPriority ����Ǿ��������������ȼ� */
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
		/* Ѱ�Ұ������������������ȼ��Ķ��� */                                                          \
		while( listLIST_IS_EMPTY( &( pxReadyTasksLists[ uxTopPriority ] ) ) )							\
		{																								\
			--uxTopPriority;																			\
		}																								\
																										\
		/* ��ȡ���ȼ���ߵľ��������TCB��Ȼ����µ�pxCurrentTCB */							            \
		listGET_OWNER_OF_NEXT_ENTRY( pxCurrentTCB, &( pxReadyTasksLists[ uxTopPriority ] ) );			\
		/* ����uxTopReadyPriority */                                                                    \
		uxTopReadyPriority = uxTopPriority;																\
	} /* taskSELECT_HIGHEST_PRIORITY_TASK */

	/*-----------------------------------------------------------*/

    /* �������궨��ֻ����ѡ���Ż�����ʱ���ã����ﶨ��Ϊ�� */
	#define taskRESET_READY_PRIORITY( uxPriority )
	#define portRESET_READY_PRIORITY( uxPriority, uxTopReadyPriority )
    
/* ����������ȼ��ľ������񣺸��ݴ������ܹ��Ż���ķ��� */
#else /* configUSE_PORT_OPTIMISED_TASK_SELECTION */

	#define taskRECORD_READY_PRIORITY( uxPriority )	portRECORD_READY_PRIORITY( uxPriority, uxTopReadyPriority )

	/*-----------------------------------------------------------*/

	#define taskSELECT_HIGHEST_PRIORITY_TASK()														    \
	{																								    \
	UBaseType_t uxTopPriority;																		    \
																									    \
		/* Ѱ�Ұ������������������ȼ��Ķ��� */								                            \
		portGET_HIGHEST_PRIORITY( uxTopPriority, uxTopReadyPriority );								    \
		/* ��ȡ���ȼ���ߵľ��������TCB��Ȼ����µ�pxCurrentTCB */                                       \
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
 * ��ϵͳʱ�������������ʱ����ʱ�б�pxDelayedTaskList ��
 * pxOverflowDelayedTaskListҪ�����л�
 */
#define taskSWITCH_DELAYED_LISTS()\
{\
	List_t *pxTemp;\
	pxTemp = pxDelayedTaskList;\
	pxDelayedTaskList = pxOverflowDelayedTaskList;\
	pxOverflowDelayedTaskList = pxTemp;\
	xNumOfOverflows++;\
	prvResetNextTaskUnblockTime();\
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
	/* �����ٽ�� */
	taskENTER_CRITICAL();
	{
		uxCurrentNumberOfTasks++;
        
        /* ���pxCurrentTCBΪ�գ���pxCurrentTCBָ���´��������� */
		if( pxCurrentTCB == NULL )
		{
			pxCurrentTCB = pxNewTCB;

			/* ����ǵ�һ�δ�����������Ҫ��ʼ��������ص��б� */
            if( uxCurrentNumberOfTasks == ( UBaseType_t ) 1 )
			{
				/* ��ʼ��������ص��б� */
                prvInitialiseTaskLists();
			}
		}
		else /* ���pxCurrentTCB��Ϊ�գ��������������ȼ���pxCurrentTCBָ��������ȼ������TCB */
		{
				if( pxCurrentTCB->uxPriority <= pxNewTCB->uxPriority )
				{
					pxCurrentTCB = pxNewTCB;
				}
		}
        
		/* ���������ӵ������б� */
        prvAddTaskToReadyList( pxNewTCB );

	}
	/* �˳��ٽ�� */
	taskEXIT_CRITICAL();
}

void vTaskStartScheduler( void )
{
    /*===================������������start==============*/
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
	/* ��ֹ�������ľ��� */
	( void ) pvParameters;
    
    for(;;)
    {
        /* ����������ʱʲô������ */
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
    
    /* ��ȡϵͳʱ��������xTickCount��ֵ */
    const TickType_t xConstTickCount = xTickCount;

    /* ������Ӿ����б����Ƴ� */
	if( vListDelet( &( pxCurrentTCB->xStateListItem ) ) == ( UBaseType_t ) 0 )
	{
		/* �����������ȼ�λͼ�ж�Ӧ��λ��� */
        portRESET_READY_PRIORITY( pxCurrentTCB->uxPriority, uxTopReadyPriority );
	}

    /* ������ʱ����ʱ��ϵͳʱ��������xTickCount��ֵ�Ƕ��� */
    xTimeToWake = xConstTickCount + xTicksToWait;

    /* ����ʱ���ڵ�ֵ����Ϊ�ڵ������ֵ */
    listSET_LIST_ITEM_VALUE( &( pxCurrentTCB->xStateListItem ), xTimeToWake );

    /* ��� */
    if( xTimeToWake < xConstTickCount )
    {
        vListInsert( pxOverflowDelayedTaskList, &( pxCurrentTCB->xStateListItem ) );
    }
    else /* û����� */
    {

        vListInsert( pxDelayedTaskList, &( pxCurrentTCB->xStateListItem ) );

        /* ������һ���������ʱ�̱���xNextTaskUnblockTime��ֵ */
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
    
    /* ɨ������б��������̵߳�remaining_tick�������Ϊ0�����1 */
	for(i=0; i<configMAX_PRIORITIES; i++)
	{
        pxTCB = ( TCB_t * ) listGET_OWNER_OF_HEAD_ENTRY( ( &pxReadyTasksLists[i] ) );
		if(pxTCB->xTicksToDelay > 0)
		{
			pxTCB->xTicksToDelay --;
            
            /* ��ʱʱ�䵽����������� */
            if( pxTCB->xTicksToDelay ==0 )
            {
                //vListInsertEnd( &( pxReadyTasksLists[i] ), &( ((TCB_t *)(&Task1TCB))->xStateListItem ) );
                taskRECORD_READY_PRIORITY( pxTCB->uxPriority );
            }
		}
	}
    
    /* �����л� */
    portYIELD();
}

#else
void xTaskIncrementTick( void )
{
	TCB_T * pxTCB;
	TickType_t xItemValue;

	const TickType_t xConstTickCount = xTickCount + 1;
	xTickCount = xConstTickCount;

	/* ���xConstTickCount��������л���ʱ�б� */
	if( xConstTickCount == ( TickType_t ) 0U )
	{
		taskSWITCH_DELAYED_LISTS();
	}

	/* �������ʱ������ʱ���� */
	if( xConstTickCount >= xNextTaskUnblockTime )
	{
		for( ;; )
		{
			if( listLIST_IS_EMPTY( pxDelayedTaskList ) != pdFALSE )
			{
				/* ��ʱ�б�Ϊ�գ�����xNextTaskUnblockTimeΪ���ܵ����ֵ */
				xNextTaskUnblockTime = portMAX_DELAY;
				break;
			}
			else /* ��ʱ�б���Ϊ�� */
			{
				pxTCB = ( TCB_T * ) listGET_OWNER_OF_HEAD_ENTRY( pxDelayedTaskList );
				xItemValue = listGET_LIST_ITEM_VALUE( &( pxTCB->xStateListItem ) );

				/* ֱ������ʱ�б���������ʱ���ڵ������Ƴ�������forѭ�� */
                if( xConstTickCount < xItemValue )
				{
					xNextTaskUnblockTime = xItemValue;
					break;
				}

				/* ���������ʱ�б��Ƴ��������ȴ�״̬ */
				( void ) vListDelet( &( pxTCB->xStateListItem ) );

				/* ������ȴ����������ӵ������б� */
				prvAddTaskToReadyList( pxTCB );
			}
		}
	}/* xConstTickCount >= xNextTaskUnblockTime */
    
    /* �����л� */
    portYIELD();
}
#endif


#else






#endif