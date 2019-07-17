/**
  ********************************  STM32F10x  *********************************
  *********************************  uC/OS-II  *********************************
 app_task.c

  ******************************************************************************/
/* 包含的头文件 --------------------------------------------------------------*/
#include "app_task.h"
#include "ucos_ii.h"
#include "led.h"
#include "stdio.h"
/*lwip头文件*/
#include "netapp/lwipconfig.h"
#include "netapp/ping.h"
#include "lwip/netif.h"
#include "lwip/opt.h"
//extern functions
extern void process_mac(void);//lwip数据输入
extern void show_address(void);//显示网口地址
extern struct netif enc28j60_netif;
/**************************************************
AppTask1

*************************************************/
void LwipInputTask(void *p_arg)
{
	/*LWIP初始化*/
	lwip_init_task();
	for(;;)
	{

		OSTimeDly(2);//延时５０个节拍
		process_mac();//轮询接收数据包
		printf("lwip.\n");
		LED1_TOGGLE;          //指示灯翻转
	}


}

/************************************************
AppTask2
*************************************************/
void AppTask2(void *p_arg)
{
  for(;;)
  {
    LED2_TOGGLE;
//    printf("running task2.\n");
//    OSTimeDly(80);
    OSTimeDlyHMSM (0,0,2,0);//时间延时２秒
  }
}

/************************************************
AppTask3
*************************************************/
void AppTask3(void *p_arg)
{
  for(;;){
    LED3_TOGGLE;
//    printf("running task3.\n");
//    OSTimeDly(100);
    OSTimeDlyHMSM (0,0,1,0);//时间延时１秒
  }
}

void AppShowAddress(void *p_arg)//该任务的调用会引起硬件中断，不明原因
{
    OSTimeDlyHMSM (0,1,0,0);//时间延时10s
	while(1)
	{
//		u32_t ip;
	    OS_CPU_SR  cpu_sr = 0u;
	    OSTimeDlyHMSM (0,1,0,0);//时间延时10s
	    OS_ENTER_CRITICAL();      //进入临界段（关中断）
//	    ip = enc28j60_netif.ip_addr.addr;
	    show_address();//显示网口地址
	    OS_EXIT_CRITICAL();       //退出临界段（开中断）
//	    printf("ip     :%d.%d.%d.%d\r\n",(uint8_t)(ip>>24),(uint8_t)(ip>>16),(uint8_t)(ip>>8),(uint8_t)(ip));
	}
}


