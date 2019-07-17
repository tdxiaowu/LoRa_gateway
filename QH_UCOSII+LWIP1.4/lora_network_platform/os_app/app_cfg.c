#include "app_cfg.h"
#include <ucos_ii.h>
#include "app_task.h"
#include "sys_tick.h"
#include "stdio.h"

/*全局变量 任务堆栈------------------------------------------------------------------*/
OS_STK TaskStartStk[TASK_START_STK_SIZE];//开始任务的栈区
OS_STK Task_LWIP_Stk[TASK_LWIP_STK_SIZE];//LWIP任务的栈区
OS_STK Task_LED1_Stk[TASK_LED1_STK_SIZE];//
OS_STK Task_LED2_Stk[TASK_LED2_STK_SIZE];
OS_STK Task_SHOW_Stk[TASK_SHOW_STK_SIZE];//显示地址任务的栈区
//操作系统滴答定时器初始化
void OSTick_Init(void)
{
	SysTick_Init();
}

/************************************************
Startup_Task
启动任务
*************************************************/
void AppTaskStart(void *p_arg)
{

#if OS_TASK_CREATE_EN > 0u
	/* 创建任务1 */
	OSTaskCreate(LwipInputTask,                                 //指向任务的指针
				  (void*)0,                                    //传递给任务的参数
				  &Task_LWIP_Stk[TASK_LWIP_STK_SIZE - 1],           //指向任务堆栈栈顶的指针
				  TASK_LWIP_PRIO);                            //任务的优先级
	/* 创建任务2 */
	OSTaskCreate(AppTask2,                                 //指向任务的指针
				(void*)0,                                       //传递给任务的参数
				  &Task_LED1_Stk[TASK_LED1_STK_SIZE-1],            //指向任务堆栈栈顶的指针
				  TASK_LED1_PRIO);                             //任务的优先级
//	/* 创建任务3 */
	OSTaskCreate(AppTask3,                                 //指向任务的指针
				(void*)0,                                       //传递给任务的参数
				&Task_LED2_Stk[TASK_LED2_STK_SIZE-1],            //指向任务堆栈栈顶的指针
				 TASK_LED2_PRIO);                             //任务的优先
	/* 创建任务4 */
	OSTaskCreate(AppShowAddress,                                 //指向任务的指针
				(void*)0,                                       //传递给任务的参数
				&Task_SHOW_Stk[TASK_SHOW_STK_SIZE-1],            //指向任务堆栈栈顶的指针
				TASK_SHOW_PRIO);

	printf("the start task is run.\n");
	OSTaskDel(OS_PRIO_SELF);
#elif OS_TASK_CREATE_EXT_EN > 0
  /* 创建任务1 */
  OSTaskCreateExt((void (*)(void *)) AppTask1,
                  (void           *) 0,
                  (OS_STK         *)&Task1_Stk[TASK1_STK_SIZE-1],
                  (INT8U           ) TASK1_PRIO,
                  (INT16U          ) TASK1_PRIO,
                  (OS_STK         *)&Task1_Stk[0],
                  (INT32U          ) TASK1_STK_SIZE,
                  (void           *) 0,
                  (INT16U          )(OS_TASK_OPT_STK_CHK | OS_TASK_OPT_STK_CLR));

  /* 创建任务2 */
  OSTaskCreateExt((void (*)(void *)) AppTask2,
                  (void           *) 0,
                  (OS_STK         *)&Task2_Stk[TASK2_STK_SIZE-1],
                  (INT8U           ) TASK2_PRIO,
                  (INT16U          ) TASK2_PRIO,
                  (OS_STK         *)&Task2_Stk[0],
                  (INT32U          ) TASK2_STK_SIZE,
                  (void           *) 0,
                  (INT16U          )(OS_TASK_OPT_STK_CHK | OS_TASK_OPT_STK_CLR));

  /* 创建任务3 */
  OSTaskCreateExt((void (*)(void *)) AppTask3,
                  (void           *) 0,
                  (OS_STK         *)&Task3_Stk[TASK3_STK_SIZE-1],
                  (INT8U           ) TASK3_PRIO,
                  (INT16U          ) TASK3_PRIO,
                  (OS_STK         *)&Task3_Stk[0],
                  (INT32U          ) TASK3_STK_SIZE,
                  (void           *) 0,
                  (INT16U          )(OS_TASK_OPT_STK_CHK | OS_TASK_OPT_STK_CLR));
#endif/*OS_TASK_CREATE_EN*/



}

//启用OS_APP_HOOKS_EN需要补的代码
void          App_TaskCreateHook      (OS_TCB          *ptcb)
{
}

