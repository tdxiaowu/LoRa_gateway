/*
 * led.h
 *
 *  Created on: Apr 3, 2018
 *      Author: zm
 */

#ifndef __LED_H_
#define __LED_H_

#include "stm32f10x.h"

/*函数声明*/
void Led_Init(void);

/*宏定义*/
#define LED_GPIO_CLK    RCC_APB2Periph_GPIOD	//LED灯的GPIO口所用时钟
#define LED_GPIO_PORT   GPIOD					//LED灯的GPIO口所用端口
#define LED1_GPIO_PIN   GPIO_Pin_6				//LED灯的GPIO口所用引脚
#define LED2_GPIO_PIN   GPIO_Pin_12
#define LED3_GPIO_PIN   GPIO_Pin_13
/*函数宏定义*/
#define LED1_ON      (LED_GPIO_PORT->BSRR = LED1_GPIO_PIN)		//LED１开
#define LED2_ON      (LED_GPIO_PORT->BSRR = LED2_GPIO_PIN)		//LED２开
#define LED3_ON      (LED_GPIO_PORT->BSRR = LED3_GPIO_PIN)		//LED３开
#define LED1_OFF     (LED_GPIO_PORT->BRR  = LED1_GPIO_PIN)    	//LED１关
#define LED2_OFF     (LED_GPIO_PORT->BRR  = LED2_GPIO_PIN)		//LED２关
#define LED3_OFF     (LED_GPIO_PORT->BRR  = LED3_GPIO_PIN)		//LED３关
#define LED1_TOGGLE  (LED_GPIO_PORT->ODR ^= LED1_GPIO_PIN)		//LED１状态翻转
#define LED2_TOGGLE  (LED_GPIO_PORT->ODR ^= LED2_GPIO_PIN)		//LED２状态翻转
#define LED3_TOGGLE  (LED_GPIO_PORT->ODR ^= LED3_GPIO_PIN)		//LED３状态翻转

#endif /* BSP_LED_H_ */
