/**
  ********************************  STM32F10x  *********************************
  *********************************  uC/OS-II  *********************************
 app_cfg.c

  ******************************************************************************/

/* 定义防止递归包含 ----------------------------------------------------------*/
#ifndef _APP_CFG_H
#define _APP_CFG_H

/* 包含的头文件 --------------------------------------------------------------*/
#include "stm32f10x.h"
#include <ucos_ii.h>
/* 宏定义 --------------------------------------------------------------------*/
/* 任务优先级 */
#define TASK_START_PRIO                        3//start任务优先级最高
#define TASK_LWIP_PRIO                         7
#define TASK_LED1_PRIO                         8
#define TASK_LED2_PRIO                         10
#define TASK_SHOW_PRIO						  12

/* 任务堆栈大小 */
#define TASK_START_STK_SIZE                   128
#define TASK_LWIP_STK_SIZE                    800
#define TASK_LED1_STK_SIZE                    64
#define TASK_LED2_STK_SIZE                    64
#define TASK_SHOW_STK_SIZE                    128

/* 任务堆栈变量 */
extern OS_STK TaskStartStk[TASK_START_STK_SIZE];


/* 函数声明 ------------------------------------------------------------------*/
void OSTick_Init(void);
void AppTaskStart(void *p_arg);

#endif /* _APP_CFG_H */



