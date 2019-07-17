/*
 * sys_tick.h
 *
 *  Created on: Apr 3, 2018
 *      Author: zm
 */

#ifndef __SYS_TICK_H_
#define __SYS_TICK_H_
#include "stm32f10x.h"

/*函数声明*/
void SysTick_Init(void);//滴答定时器初始化
//void TimeDelay(uint32_t time);//延时函数单位1毫秒


///*宏定义*/
//#define  TICKS_PER_SEC 50   //每秒50个滴答,每20毫秒中断一次

#endif /* BSP_SYS_TICK_H_ */
