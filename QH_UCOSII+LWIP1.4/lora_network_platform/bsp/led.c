/*
 * led.c
 *
 *  Created on: Apr 3, 2018
 *      Author: zm
 */
#include "led.h"
/*LED_GPIO初始化
 * 功能描述：LED灯GPIO口初始化；
 * 　　　　　输出类型：推挽输出；
 * 　　　　　输出速度：2MHz；
 * 		　　初始化后，GPIO口输出为低电平．
 * */
void Led_Init(void)
{
	RCC_APB2PeriphClockCmd(LED_GPIO_CLK, ENABLE);      //使能LED时钟

	GPIO_InitTypeDef  GPIO_InitStructure;              //定义GPIO初始化结构体变量
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;   //推挽输出
	GPIO_InitStructure.GPIO_Pin = LED1_GPIO_PIN | LED2_GPIO_PIN | LED3_GPIO_PIN;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;   //输出速度
	GPIO_Init(LED_GPIO_PORT,&GPIO_InitStructure);      //初始化GPIO

	GPIO_ResetBits(LED_GPIO_PORT, LED1_GPIO_PIN | LED2_GPIO_PIN | LED3_GPIO_PIN);//输出低电平
}



