/*
*********************************************************************************************************
*                                                uC/OS-II
*                                          The Real-Time Kernel
*                                  uC/OS-II Configuration File for V2.8x
*
*                               (c) Copyright 2005-2009, Micrium, Weston, FL
*                                          All Rights Reserved
*
*
* File    : OS_CFG.H
* By      : Jean J. Labrosse
* Version : V2.88
*
* LICENSING TERMS:
* ---------------
*   uC/OS-II is provided in source form for FREE evaluation, for educational use or for peaceful research.
* If you plan on using  uC/OS-II  in a commercial product you need to contact Micri�m to properly license
* its use in your product. We provide ALL the source code for your convenience and to help you experience
* uC/OS-II.   The fact that the  source is provided does  NOT  mean that you can use it without  paying a
* licensing fee.
*********************************************************************************************************
*/

#ifndef OS_CFG_H
#define OS_CFG_H


                                       /* ---------------------- MISCELLANEOUS　miscellaneous 杂项 ----------------------- */
#define OS_APP_HOOKS_EN           0    /* Application-defined hooks are called from the uC/OS-II hooks  使能APP应用HOOK函数 */
#define OS_ARG_CHK_EN             0    /* Enable (1) or Disable (0) argument checking     参数检查             */
#define OS_CPU_HOOKS_EN           1    /* uC/OS-II hooks are found in the processor port files  使能OS_CPU HOOK函数       */

#define OS_DEBUG_EN               0    /* Enable(1) debug variables    使能系统调试功能                                */

#define OS_EVENT_MULTI_EN         0    /* Include code for OSEventPendMulti()     使用OSEventPendMulti()函数                      */
#define OS_EVENT_NAME_EN          0    /* Enable names for Sem, Mutex, Mbox and Q    使能事件(Sem, Mutex, Mbox and Q)名称                  */

#define OS_LOWEST_PRIO           63    /* Defines the lowest priority that can be assigned ...     定义最低优先级(值) ...     */
                                       /* ... MUST NEVER be higher than 254!                           */

#define OS_MAX_EVENTS             50    /* Max. number of event control blocks in your application 在应用程序中事件控制块的最大数量     */
#define OS_MAX_FLAGS              5    /* Max. number of Event Flag Groups    in your application  信号量集数目 */
#define OS_MAX_MEM_PART           5    /* Max. number of memory partitions 内存分区的数量   */
#define OS_MAX_QS                 5    /* Max. number of queue control blocks in your application  消息队列控制块数量 */
#define OS_MAX_TASKS             10    /* Max. number of tasks in your application, MUST be >= 2  最大任务数     */

#define OS_SCHED_LOCK_EN         1u    /* Include code for OSSchedLock() and OSSchedUnlock()  系统调度锁开关 */

#define OS_TICK_STEP_EN           0    /* Enable tick stepping feature for uC/OS-View                  */
#define OS_TICKS_PER_SEC        200    /* Set the number of ticks in one second    每秒多少的节拍 10ms        */


                                       /* --------------------- TASK STACK SIZE ---------------------- */
#define OS_TASK_TMR_STK_SIZE    128    /* Timer      task stack size (# of OS_STK wide entries) 定时器任务堆栈       */
#define OS_TASK_STAT_STK_SIZE   128    /* Statistics task stack size (# of OS_STK wide entries) 统计任务堆栈       */
#define OS_TASK_IDLE_STK_SIZE   128    /* Idle       task stack size (# of OS_STK wide entries) 空闲任务堆栈       */


                                       /* --------------------- TASK MANAGEMENT ---------------------- */
#define OS_TASK_CHANGE_PRIO_EN    1    /*     Include code for OSTaskChangePrio()    任务优先级切换                  */
#define OS_TASK_CREATE_EN         1    /*     Include code for OSTaskCreate()        任务创建                  */
#define OS_TASK_CREATE_EXT_EN     0    /*     Include code for OSTaskCreateExt()     扩展任务创建                  */
#define OS_TASK_DEL_EN            1    /*     Include code for OSTaskDel()           删除任务                  */
#define OS_TASK_NAME_EN           1    /*     Enable task names    使用OSTaskNameGet()和OSTaskNameSet()--- Get,Set任务名称                  */
#define OS_TASK_PROFILE_EN        0    /*     Include variables in OS_TCB for profiling  包含OS_TCB性能分析              */
#define OS_TASK_QUERY_EN          1    /*     Include code for OSTaskQuery()      查询任务(信息)                     */
#define OS_TASK_STAT_EN           0    /*     Enable (1) or Disable(0) the statistics task     使能统计任务         */
#define OS_TASK_STAT_STK_CHK_EN   0    /*     Check task stacks from statistic task       使能检查任务堆栈             */
#define OS_TASK_SUSPEND_EN        1    /*     Include code for OSTaskSuspend() and OSTaskResume()   任务挂起、继续   */
#define OS_TASK_SW_HOOK_EN        1    /*     Include code for OSTaskSwHook()     任务切换HOOK函数                     */
#define OS_TASK_REG_TBL_SIZE      0    /*     Size of task variables array (#of INT32U entries)  任务数组变量的大小 (#of INT32U entries)      */


                                       /* ----------------------- EVENT FLAGS ------------------------ */
#define OS_FLAG_EN                0    /* Enable (1) or Disable (0) code generation for EVENT FLAGS 使能OS_FLAG事件标志所有功能   */
#define OS_FLAG_ACCEPT_EN         1    /*     Include code for OSFlagAccept()        检查(获取)事件状态                    */
#define OS_FLAG_DEL_EN            1    /*     Include code for OSFlagDel()           删除事件                  */
#define OS_FLAG_NAME_EN           1    /*     Enable names for event flag group       使用事件标志名称                  */
#define OS_FLAG_QUERY_EN          1    /*     Include code for OSFlagQuery()          查询事件                */
#define OS_FLAG_WAIT_CLR_EN       1    /* Include code for Wait on Clear EVENT FLAGS   使能事件等待清除功能                */
#define OS_FLAGS_NBITS           16    /* Size in #bits of OS_FLAGS data type (8, 16 or 32)  定义OS_FLAGS数据类型(8, 16 or 32)          */


                                       /* -------------------- MESSAGE MAILBOXES 消息邮箱--------------------- */
#define OS_MBOX_EN                1    /* Enable (1) or Disable (0) code generation for MAILBOXES      */
#define OS_MBOX_ACCEPT_EN         1    /*     Include code for OSMboxAccept()                          */
#define OS_MBOX_DEL_EN            1    /*     Include code for OSMboxDel()                             */
#define OS_MBOX_PEND_ABORT_EN     1    /*     Include code for OSMboxPendAbort()                       */
#define OS_MBOX_POST_EN           1    /*     Include code for OSMboxPost()                            */
#define OS_MBOX_POST_OPT_EN       1    /*     Include code for OSMboxPostOpt()                         */
#define OS_MBOX_QUERY_EN          1    /*     Include code for OSMboxQuery()                           */


                                       /* --------------------- MEMORY MANAGEMENT 内存管理-------------------- */
#define OS_MEM_EN                 1    /* Enable (1) or Disable (0) code generation for MEMORY MANAGER */
#define OS_MEM_NAME_EN            1    /*     Enable memory partition names   使能内存分区命名        */
#define OS_MEM_QUERY_EN           1    /*     Include code for OSMemQuery()                            */


                                       /* ---------------- MUTUAL EXCLUSION SEMAPHORES 互斥信号量--------------- */
#define OS_MUTEX_EN               1    /* Enable (1) or Disable (0) code generation for MUTEX          */
#define OS_MUTEX_ACCEPT_EN        1    /*     Include code for OSMutexAccept()                         */
#define OS_MUTEX_DEL_EN           1    /*     Include code for OSMutexDel()                            */
#define OS_MUTEX_QUERY_EN         1    /*     Include code for OSMutexQuery()                          */


                                       /* ---------------------- MESSAGE QUEUES 消息队列---------------------- */
#define OS_Q_EN                   1    /* Enable (1) or Disable (0) code generation for QUEUES         */
#define OS_Q_ACCEPT_EN            1    /*     Include code for OSQAccept()                             */
#define OS_Q_DEL_EN               1    /*     Include code for OSQDel()                                */
#define OS_Q_FLUSH_EN             1    /*     Include code for OSQFlush()                              */
#define OS_Q_PEND_ABORT_EN        1    /*     Include code for OSQPendAbort()                          */
#define OS_Q_POST_EN              1    /*     Include code for OSQPost()                               */
#define OS_Q_POST_FRONT_EN        1    /*     Include code for OSQPostFront()                          */
#define OS_Q_POST_OPT_EN          1    /*     Include code for OSQPostOpt()                            */
#define OS_Q_QUERY_EN             1    /*     Include code for OSQQuery()                              */


                                       /* ------------------------ SEMAPHORES 信号量------------------------ */
#define OS_SEM_EN                 1    /* Enable (1) or Disable (0) code generation for SEMAPHORES     */
#define OS_SEM_ACCEPT_EN          1    /*    Include code for OSSemAccept()                            */
#define OS_SEM_DEL_EN             1    /*    Include code for OSSemDel()                               */
#define OS_SEM_PEND_ABORT_EN      1    /*    Include code for OSSemPendAbort()                         */
#define OS_SEM_QUERY_EN           1    /*    Include code for OSSemQuery()                             */
#define OS_SEM_SET_EN             1    /*    Include code for OSSemSet()                               */


                                       /* --------------------- TIME MANAGEMENT 时间管理---------------------- */
#define OS_TIME_DLY_HMSM_EN       1    /*     Include code for OSTimeDlyHMSM()                         */
#define OS_TIME_DLY_RESUME_EN     0    /*     Include code for OSTimeDlyResume()                       */
#define OS_TIME_GET_SET_EN        1    /*     Include code for OSTimeGet() and OSTimeSet()             */
#define OS_TIME_TICK_HOOK_EN      1    /*     Include code for OSTimeTickHook()                        */


                                       /* --------------------- TIMER MANAGEMENT 定时器管理--------------------- */
#define OS_TMR_EN                 0    /* Enable (1) or Disable (0) code generation for TIMERS         */
#define OS_TMR_CFG_MAX           16    /*     Maximum number of timers                                 */
#define OS_TMR_CFG_NAME_EN        0    /*     Determine timer names                                    */
#define OS_TMR_CFG_WHEEL_SIZE     7    /*     Size of timer wheel (#Spokes)                            */
#define OS_TMR_CFG_TICKS_PER_SEC 10    /*     Rate at which timer management task runs (Hz)            */

#endif
