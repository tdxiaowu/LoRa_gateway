//lwip版本为1.4.1
/*
 * SRC_IP:192.168.0.18
 * DES_IP:192,168,0,103
 */
/*stm32库头文件*/
#include "stm32f10x.h"
/*bsp头文件*/
#include "sys_tick.h"
#include "led.h"
#include "usart1.h"
/*ucos头文件*/
#include <ucos_ii.h>
#include "app_cfg.h"
#include "app_task.h"
/*lwip头文件*/
#include "netapp/lwipconfig.h"
#include "netapp/http_server.h"
#include "netapp/ping.h"
#include "spi.h"
/*Ｃ库头文件*/
#include <stdio.h>



/************************/
int main(void)
{

#if !NO_SYS

	/*-----BSP硬件初始化----*/
	/*LED驱动初始化*/
	Led_Init();
	/*异步串口1驱动初始化*/
	USART1_InitConfig();
	/*SPI通信接口初始化*/
	SPI1_Init();

	/*OS操作系统*/
	OSInit();
	/*操作系统时钟初始化*/
	OSTick_Init();

#if LWIP_TCP
	httpserver_create();//创建HTTP Server任务
#endif
	/*ping功能初始化设置*/
//	ping_init();

	/* 创建任务 */
#if OS_TASK_CREATE_EN > 0u
	OSTaskCreate(AppTaskStart,                                 //指向任务的指针
				  (void*)0,                                      //指向任务参数的指针
				  &TaskStartStk[TASK_START_STK_SIZE-1],        //指向任务堆栈栈顶的指针
				  TASK_START_PRIO);                            //任务的优先级

#elif OS_TASK_CREATE_EXT_EN > 0
	OSTaskCreateExt((void (*)(void *)) AppTaskStart,
					  (void           *) 0,
					  (OS_STK         *)&TaskStartStk[TASK_START_STK_SIZE-1],
					  (INT8U           ) TASK_START_PRIO,
					  (INT16U          ) TASK_START_PRIO,
					  (OS_STK         *)&TaskStartStk[0],
					  (INT32U          ) TASK_START_STK_SIZE,
					  (void           *) 0,
					  (INT16U          )(OS_TASK_OPT_STK_CHK | OS_TASK_OPT_STK_CLR));

#endif

	printf("*    MCU:STM32F103VET6    *\r\n");
	printf("*    ucosII + lwip1.4.1   *\r\n");
	printf("**********OS start*********\r\n");
	/*启动任务*/
	OSStart();
	/*主函数体*/
#else
	//无操作系统时，lwip初始化
	/*-----硬件初始化----*/
	/*LED驱动初始化*/
	Led_Init();
	/*异步串口1驱动初始化*/
	USART1_InitConfig();
	/*SPI通信接口初始化*/
	SPI1_Init();
	/*滴答定时器初始化*/
	SysTick_Init();


	printf("* MCU:STM32F103VET6 *\r\n");
	printf("*    ENC28J60   *\r\n");
	printf("***************************************************\r\n");

	lwip_task(NULL);
#endif
	return 0;
}
