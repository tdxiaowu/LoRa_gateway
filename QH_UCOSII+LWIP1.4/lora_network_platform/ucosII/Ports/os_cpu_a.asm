@********************************************************************************************************
@                                               uC/OS-II
@                                         The Real-Time Kernel
@
@                               (c) Copyright 1992-2006, Micrium, Weston, FL
@                                          All Rights Reserved
@
@                                           ARM Cortex-M3 Port
@
@ File      : OS_CPU_A.ASM
@ Version   : V2.89
@ By        : Jean J. Labrosse
@             Brian Nagel
@
@ For       : ARMv7M Cortex-M3
@ Mode      : Thumb2
@ Toolchain : GNU C Compiler
@********************************************************************************************************

@********************************************************************************************************
@                                           PUBLIC FUNCTIONS
@********************************************************************************************************

    .extern  OSRunning                                           @ External references
    .extern  OSPrioCur
    .extern  OSPrioHighRdy
    .extern  OSTCBCur
    .extern  OSTCBHighRdy
    .extern  OSIntExit
    .extern  OSTaskSwHook
    .extern  OS_CPU_ExceptStkBase


    .global  OS_CPU_SR_Save                                      @ Functions declared in this file
    .global  OS_CPU_SR_Restore
    .global  OSStartHighRdy
    .global  OSCtxSw
    .global  OSIntCtxSw
    .global  PendSV_Handler      //将OS_CPU_PendSVHandler改为PendSV_Handler

@********************************************************************************************************
@                                                EQUATES
@********************************************************************************************************

.equ NVIC_INT_CTRL,   0xE000ED04                              @ Interrupt control state register.中断控制状态寄存器地址
.equ NVIC_SYSPRI14,   0xE000ED22                              @ System priority register (priority 14).系统优先级寄存器
.equ NVIC_PENDSV_PRI, 0xFF                                    @ PendSV priority value (lowest).PendSV优先级（最低）
.equ NVIC_PENDSVSET,  0x10000000                              @ Value to trigger PendSV exception.该值能够触发PendSV异常

@********************************************************************************************************
@                                      CODE GENERATION DIRECTIVES
@********************************************************************************************************

.text
.align 2
.thumb
.syntax unified

@********************************************************************************************************
@                                   CRITICAL SECTION METHOD 3 FUNCTIONS
@
@ Description: Disable/Enable interrupts by preserving the state of interrupts.  Generally speaking you
@              would store the state of the interrupt disable flag in the local variable 'cpu_sr' and then
@              disable interrupts.  'cpu_sr' is allocated in all of uC/OS-II's functions that need to
@              disable interrupts.  You would restore the interrupt disable state by copying back 'cpu_sr'
@              into the CPU's status register.
@
@ Prototypes :     OS_CPU_SR  OS_CPU_SR_Save(void);
@                  void       OS_CPU_SR_Restore(OS_CPU_SR cpu_sr);
@
@
@ Note(s)    : 1) These functions are used in general like this:
@
@                 void Task (void *p_arg)
@                 {
@                 #if OS_CRITICAL_METHOD == 3          /* Allocate storage for CPU status register */
@                     OS_CPU_SR  cpu_sr;
@                 #endif
@
@                          :
@                          :
@                     OS_ENTER_CRITICAL();             /* cpu_sr = OS_CPU_SaveSR();                */
@                          :
@                          :
@                     OS_EXIT_CRITICAL();              /* OS_CPU_RestoreSR(cpu_sr);                */
@                          :
@                          :
@                 }
@********************************************************************************************************
.thumb_func
OS_CPU_SR_Save:
    MRS     R0, PRIMASK                                         @ Set prio int mask to mask all (except faults)
    CPSID   I
    BX      LR

.thumb_func
OS_CPU_SR_Restore:
    MSR     PRIMASK, R0
    BX      LR

@********************************************************************************************************
@                                          START MULTITASKING　　　　　　开始多任务系统
@                                       void OSStartHighRdy(void)
@
@ Note(s) : 1) This function triggers a PendSV exception (essentially, causes a context switch) to cause
@              the first task to start.
@
@           2) OSStartHighRdy() MUST:
@              a) Setup PendSV exception priority to lowest;
@              b) Set initial PSP to 0, to tell context switcher this is first run;
@              c) Set the main stack to OS_CPU_ExceptStkBase
@              d) Set OSRunning to TRUE;
@              e) Trigger PendSV exception;
@              f) Enable interrupts (tasks will run with interrupts enabled).
@********************************************************************************************************
@这部分的代码是任务在第一次运行的时候，设置PendSV中断以及中断优先级，同时开启PendSV中断，进行第一个任务切换。
.thumb_func
OSStartHighRdy:
    LDR     R0, =NVIC_SYSPRI14                                  @ Set the PendSV exception priority 设置PendSV的优先级为0XFF，为最低优先级
    LDR     R1, =NVIC_PENDSV_PRI
    STRB    R1, [R0]

    MOVS    R0, #0                                              @ Set the PSP to 0 for initial context switch call
    MSR     PSP, R0

    LDR     R0, =OS_CPU_ExceptStkBase                           @ Initialize the MSP to the OS_CPU_ExceptStkBase
    LDR     R1, [R0]
    MSR     MSP, R1    

    LDR     R0, =OSRunning                                      @ OSRunning = TRUE　设置OSRunning为１表示系统核在运行
    MOVS    R1, #1
    STRB    R1, [R0]
    
    LDR     R0, =NVIC_INT_CTRL                                  @ Trigger the PendSV exception (causes context switch)
    LDR     R1, =NVIC_PENDSVSET
    STR     R1, [R0]

    CPSIE   I                                                   @ Enable interrupts at processor level

OSStartHang:
    B       OSStartHang                                         @ Should never get here


@********************************************************************************************************
@                               PERFORM A CONTEXT SWITCH (From task level)任务级上下文切换
@                                           void OSCtxSw(void)
@在UCOS-II中对任务进行切换的时候，调用了OS_TSAK_SW()函数，这个函数也是用汇编写成的。这个函数人为的模仿了一次中断。
@调用PendSV异常，对任务实现了切换。
@ Note(s) : 1) OSCtxSw() is called when OS wants to perform a task context switch.  This function
@              triggers the PendSV exception which is where the real work is done.
@********************************************************************************************************

.thumb_func
OSCtxSw:
    LDR     R0, =NVIC_INT_CTRL       @ Trigger the PendSV exception (causes context switch)把NVIC_INT_CTRL的值赋值给R0
    LDR     R1, =NVIC_PENDSVSET     @把NVIC_PENDSVSET赋值给R1
    STR     R1, [R0]    @把R1的值赋值到R0指向的地址中，即*R0 = R1
    BX      LR
@上面的函数运行结束后，已经触发了一个pendsv异常
@********************************************************************************************************
@                             PERFORM A CONTEXT SWITCH (From interrupt level)中断级上下文切换
@                                         void OSIntCtxSw(void)
@
@ Notes:    1) OSIntCtxSw() is called by OSIntExit() when it determines a context switch is needed as
@              the result of an interrupt.  This function simply triggers a PendSV exception which will
@              be handled when there are no more interrupts active and interrupts are enabled.
@********************************************************************************************************

.thumb_func
OSIntCtxSw:
    LDR     R0, =NVIC_INT_CTRL                                  @ Trigger the PendSV exception (causes context switch)引发上下文切换
    LDR     R1, =NVIC_PENDSVSET
    STR     R1, [R0]
    BX      LR

@********************************************************************************************************
@                                         HANDLE PendSV EXCEPTION　　处理PendSV异常
@                                     void OS_CPU_PendSVHandler(void)
@
@ Note(s) : 1) PendSV is used to cause a context switch.  This is a recommended method for performing
@              context switches with Cortex-M3.  This is because the Cortex-M3 auto-saves half of the
@              processor context on any exception, and restores same on return from exception.  So only
@              saving of R4-R11 is required and fixing up the stack pointers.  Using the PendSV exception
@              this way means that context saving and restoring is identical whether it is initiated from
@              a thread or occurs due to an interrupt or exception.
@
@           2) Pseudo-code is:
@              a) Get the process SP, if 0 then skip (goto d) the saving part (first context switch);
@              b) Save remaining regs r4-r11 on process stack;
@              c) Save the process SP in its TCB, OSTCBCur->OSTCBStkPtr = SP;
@              d) Call OSTaskSwHook();
@              e) Get current high priority, OSPrioCur = OSPrioHighRdy;
@              f) Get current ready thread TCB, OSTCBCur = OSTCBHighRdy;
@              g) Get new process SP from TCB, SP = OSTCBHighRdy->OSTCBStkPtr;
@              h) Restore R4-R11 from new process stack;
@              i) Perform exception return which will restore remaining context.
@
@           3) On entry into PendSV handler:
@              a) The following have been saved on the process stack (by processor):
@                 xPSR, PC, LR, R12, R0-R3
@              b) Processor mode is switched to Handler mode (from Thread mode)
@              c) Stack is Main stack (switched from Process stack)
@              d) OSTCBCur      points to the OS_TCB of the task to suspend
@                 OSTCBHighRdy  points to the OS_TCB of the task to resume
@
@           4) Since PendSV is set to lowest priority in the system (by OSStartHighRdy() above), we
@              know that it will only be run when no other exception or interrupt is active, and
@              therefore safe to assume that context being switched out was using the process stack (PSP).
@********************************************************************************************************
//修改，将OS_CPU_PendSVHandler改为PendSV_Handler方便stm32硬件中断的调用
.thumb_func
PendSV_Handler:
    CPSID   I                                                   @ Prevent interruption during context switch
    MRS     R0, PSP                                             @ PSP is process stack pointer
    CBZ     R0, OS_CPU_PendSVHandler_nosave                     @ Skip register save the first time　如果是第一次任务切换．直接跳转，不进行下面的部分

    SUBS    R0, R0, #0x20                                       @ Save remaining regs r4-11 on process stack
    @因为CM3是向下满栈的，即向下增长的，每入栈一字节之前，sp每次减４；这里需要入栈，R4-R11为32位的寄存器,8*32/8=32
    STM     R0, {R4-R11}        @一次把R4-R11的值放到R0指向的堆栈

    LDR     R1, =OSTCBCur                                       @ OSTCBCur->OSTCBStkPtr = SP;
    LDR     R1, [R1]         @*R1=R1
    STR     R0, [R1]         @*R1=R0                                   @ R0 is SP of process being switched out

                                                                @ At this point, entire context of process has been saved
OS_CPU_PendSVHandler_nosave:
    PUSH    {R14}                                               @ Save LR exc_return value
    LDR     R0, =OSTaskSwHook                                   @ OSTaskSwHook();
    BLX     R0
    POP     {R14}

    LDR     R0, =OSPrioCur                                      @ OSPrioCur = OSPrioHighRdy;
    LDR     R1, =OSPrioHighRdy
    LDRB    R2, [R1]
    STRB    R2, [R0]

    LDR     R0, =OSTCBCur                                       @ OSTCBCur  = OSTCBHighRdy;
    LDR     R1, =OSTCBHighRdy
    LDR     R2, [R1]
    STR     R2, [R0]

    LDR     R0, [R2]                                            @ R0 is new process SP; SP = OSTCBHighRdy->OSTCBStkPtr;
    LDM     R0, {R4-R11}                                        @ Restore r4-11 from new process stack
    ADDS    R0, R0, #0x20
    MSR     PSP, R0                                             @ Load PSP with new process SP
    ORR     LR, LR, #0x04                                       @ Ensure exception return uses process stack
    CPSIE   I
    BX      LR                                                  @ Exception return will restore remaining context

.end
@在上面的代码中主要讲述了使用PendSV中断来对两个任务之间进行切换。主要做的工作是对两个任务的各个寄存器进行PUSH及POP。同时第一个任务切换的时候不需要保存各个寄存器的值。
