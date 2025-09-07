/*
********************************************************
*                     ??????????
********************************************************
*/
#include "task.h"
#include "FreeRTOS.h"


/*
********************************************************
*                     ??????
********************************************************
*/
portCHAR flag1;
portCHAR flag2;
portCHAR flag3 = 0;

struct xLIST        List_Test;


struct xList_ITEM   List_Item1;
struct xList_ITEM   List_Item2;
struct xList_ITEM   List_Item3;


extern List_t pxReadyTasksLists[ configMAX_PRIORITIES ];

/*
********************************************************
*                     ????????
********************************************************
*/
TaskHandle_t Task1_Handle;
#define TASK1_STACK_SIZE       128
StackType_t Task1Stack[TASK1_STACK_SIZE];
TCB_T Task1TCB;


TaskHandle_t Task2_Handle;
#define TASK2_STACK_SIZE       128
StackType_t Task2Stack[TASK2_STACK_SIZE];
TCB_T Task2TCB;

TaskHandle_t xIdleTaskHandle;
TCB_T IdleTaskTCB;





/*
********************************************************
*                     ????????
********************************************************
*/

void Task1_Entry(void *p_arg);
void Task2_Entry(void *p_arg);



void HardWareInit(void)
{
  /**/
}

void RTOSInit(void)
{
  /**/
}

void RTOSStart(void)
{
  /**/
}

void DoSomething1(void)
{
    /*????????????????*/
    for(;;){
      
        if(flag1){
          
        }
    }
}

void DoSomething2(void)
{
    /*????????????????*/
    for(;;){
      
        if(flag2){
          
        }
    }
}

void DoSomething3(void)
{
    /*????????????????*/
    for(;;){
      
        if(flag3){
          
        }
    }
}


/*
**************************************************
*                 main ????
*/
int main()
{
    HardWareInit();

  
    RTOSInit();
  
    RTOSStart();
    /* vListInitialise(&List_Test);
  
    vListInitialistItem(&List_Item1);
    List_Item1.NodeTick = 1;
  
    vListInitialistItem(&List_Item2);
    List_Item2.NodeTick = 2;
  
    vListInitialistItem(&List_Item3);
    List_Item3.NodeTick = 3;
  
    vListInset(&List_Test, &List_Item2);
    vListInset(&List_Test, &List_Item1);
    vListInset(&List_Test, &List_Item3);*/
    prvInitialiseTaskLists();
    
    Task1_Handle = 
    xTaskCreateStatic( (TaskFunction_t)Task1_Entry,
                        (char *)"Task1",
                        (uint32_t)TASK1_STACK_SIZE,
                        (void *)NULL,
                         1,
                        (StackType_t *)Task1Stack,
                        (TCB_T *)&Task1TCB);
     
     /*vListInsertEnd( &( pxReadyTasksLists[1] ),
                     &( ((TCB_T*)(&Task1TCB))->xStateListItem ));*/


    Task2_Handle = 
    xTaskCreateStatic( (TaskFunction_t)Task2_Entry,
                        (char *)"Task2",
                        (uint32_t)TASK2_STACK_SIZE,
                        (void *)NULL,
                         2,
                        (StackType_t *)Task2Stack,
                        (TCB_T *)&Task2TCB);
     
     /*vListInsertEnd( &( pxReadyTasksLists[2] ),
                     &( ((TCB_T*)(&Task2TCB))->xStateListItem ));*/
                        
     vTaskStartScheduler();
                        
     for(;;)
     {
       
     }

}

void Task1_Entry( void *parg )
{
    for(;;)
    {
#if 0
        flag1 = 1;
        delay(100);
        flag1 = 0;
        delay(100);
      
        taskYIELD();
#else
        flag1 = 1;
        vTaskDelay(2);
        flag1 = 0;
        vTaskDelay(2);
#endif
    }
}


void Task2_Entry( void *parg )
{
    for(;;)
    {
#if 0
        flag2 = 1;
        delay(100);
        flag2 = 0;
        delay(100);
      
        taskYIELD();
#else
        flag2 = 1;
        vTaskDelay(2);
        flag2 = 0;
        vTaskDelay(2);
        
#endif
    }
}