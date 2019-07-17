/*
 * sys_tick.c
 *
 *  Created on: Apr 3, 2018
 *      Author: zm
 */
#include "sys_tick.h"
#include "os_cfg.h"

static __IO uint32_t Delay_times;
void SysTick_Init(void)
{
	RCC_ClocksTypeDef  RCC_Clockstructure;
	RCC_GetClocksFreq(&RCC_Clockstructure);//获取系统时钟

	SysTick_Config(RCC_Clockstructure.HCLK_Frequency / OS_TICKS_PER_SEC);//滴答定时器配置和使能中断,10毫秒
}
/*
 * 延时函数
 * 参数：延时时间（ms）
 * */
void TimeDelay(uint32_t time)
{
	//SysTick_Init();//滴答定时器初始化
	Delay_times = time;
	while(Delay_times != 0);

	//SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;//关闭滴答定时器
}

/*时间递减函数*/
void TimeDecline(void)
{
	if(Delay_times != 0x00)
	{
		Delay_times--;
	}

}

/*无操作系统时使用
 * 用于时间延时
 * */

//void SysTick_Handler(void)
//{
//	TimeDecline();
//}





