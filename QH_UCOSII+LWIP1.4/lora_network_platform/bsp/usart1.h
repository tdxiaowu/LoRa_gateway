/*
 * usart1.h
 *
 *  Created on: Apr 19, 2018
 *      Author: zm
 */

#ifndef __USART1_H
#define __USART1_H

#include "stm32f10x.h"
#include <stdarg.h>

//void USART1_NVIC_Config(void);
#define  USARTx                 USART1
#define  USARTx_IRQn            USART1_IRQn
#define  USART1_BaudRate        115200   //波特率
#define  USART1_GPIO_PORT       GPIOA   //串口端口
#define  USART1_GPIO_PIN_TX     GPIO_Pin_9 //串口发送引脚
#define  USART1_GPIO_PIN_RX     GPIO_Pin_10//串口接收引脚

void USART1_InitConfig(void);//串口1初始化配置
void USART_SendByte(USART_TypeDef* pUSARTx,uint8_t ch);//发送一个字节数据
void USART_SendString(USART_TypeDef* pUSARTx,char *str);//发送一个字符串
void USART1_printf(USART_TypeDef* pUSARTx, uint8_t *Data,...);//格式化输出，类似于C库中的print



#endif /* HADWARE_USART_H_ */
