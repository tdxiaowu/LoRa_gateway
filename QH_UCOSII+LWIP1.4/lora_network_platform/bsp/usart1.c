/*
 * usart1.c
 *
 *  Created on: Apr 19, 2018
 *      Author: zm
 */
#include "usart1.h"

/*串口1中断优先级配置*/
static void USART1_NVIC_Config(void)
{
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);//中断优先级分组为2

	NVIC_InitTypeDef USART1_NVIC_InitStructure;
	USART1_NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;//确定中断源
	USART1_NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;//主优先级为1
	USART1_NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;//次优先级为1
	USART1_NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;//使能中断源
	NVIC_Init(&USART1_NVIC_InitStructure);
}
/*串口1初始化配置(含中断)*/
void USART1_InitConfig(void)
{
	/*GPIO口初始化PA9:USART1_TX,PA10:USART1_RX*/
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);//A口时钟使能

	GPIO_InitTypeDef GPIOA_GPIO_InitStructure;
	GPIOA_GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;//复用推挽输出模式
	GPIOA_GPIO_InitStructure.GPIO_Pin = USART1_GPIO_PIN_TX;
	GPIOA_GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(USART1_GPIO_PORT, &GPIOA_GPIO_InitStructure);

	GPIOA_GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;//浮空输入模式
	GPIOA_GPIO_InitStructure.GPIO_Pin = USART1_GPIO_PIN_RX;
	GPIO_Init(USART1_GPIO_PORT, &GPIOA_GPIO_InitStructure);

	/*初始化异步串口1*/
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);//串口1时钟使能

	USART_InitTypeDef  USART1_USART_InitStructure;
	USART1_USART_InitStructure.USART_BaudRate = USART1_BaudRate;//设置波特率
	USART1_USART_InitStructure.USART_WordLength = USART_WordLength_8b;//设置字长8位
	USART1_USART_InitStructure.USART_StopBits = USART_StopBits_1;//停止位1位
	USART1_USART_InitStructure.USART_Parity = USART_Parity_No;//奇偶校验:无
	USART1_USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;//使能串口接受和发送
	USART1_USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//硬件流控制无
	USART_Init(USARTx, &USART1_USART_InitStructure);

	/*串口中断优先级配置*/
	USART1_NVIC_Config();
	/*串口接收中断配置*/
	USART_ITConfig(USARTx, USART_IT_RXNE, ENABLE);
	/*使能串口*/
	USART_Cmd(USART1, ENABLE);
}

/*发送一个字节的数据*/
void USART_SendByte(USART_TypeDef* pUSARTx,uint8_t ch)
{
	USART_SendData(pUSARTx, ch);//发送一个字节数据到USART
	/*等待发送数据寄存器为空*/
	while(USART_GetFlagStatus(pUSARTx,USART_FLAG_TXE) == RESET);
}

/*发送一个字符串*/
void USART_SendString(USART_TypeDef* pUSARTx,char *str)
{
	uint16_t k = 0;
	do{
		USART_SendByte(pUSARTx,*(str + k));
		k++;
	}while(*(str + k)!= '\0');//直到字符串最后一位0
	/*等待字符串发送完成*/
	while(USART_GetFlagStatus(pUSARTx,  USART_FLAG_TC) == RESET);
}

/*
 * 函数名：itoa
 * 描述  ：将整形数据转换成字符串
 * 输入  ：-radix =10 表示10进制，其他结果为0
 *         -value 要转换的整形数
 *         -buf 转换后的字符串
 *         -radix = 10
 * 输出  ：无
 * 返回  ：无
 * 调用  ：被USART1_printf()调用
 */
static char *itoa(int value, char *string, int radix)
{
	int     i, d;
	int     flag = 0;
	char    *ptr = string;

	/* This implementation only works for decimal numbers. */
	if (radix != 10)
	{
	    *ptr = 0;
	    return string;
	}

	if (!value)
	{
	    *ptr++ = 0x30;
	    *ptr = 0;
	    return string;
	}

	/* if this is a negative value insert the minus sign. */
	if (value < 0)
	{
	    *ptr++ = '-';

	    /* Make the value positive. */
	    value *= -1;
	}

	for (i = 10000; i > 0; i /= 10)
	{
	    d = value / i;

	    if (d || flag)
	    {
	        *ptr++ = (char)(d + 0x30);
	        value -= (d * i);
	        flag = 1;
	    }
	}

	/* Null terminate the string. */
	*ptr = 0;

	return string;

} /* NCL_Itoa */

/*
 * 函数名：USART1_printf
 * 描述  ：格式化输出，类似于C库中的printf，但这里没有用到C库
 * 输入  ：-USARTx 串口通道，这里只用到了串口1，即USART1
 *		     -Data   要发送到串口的内容的指针
 *			   -...    其他参数
 * 输出  ：无
 * 返回  ：无
 * 调用  ：外部调用
 *         典型应用USART1_printf( USART1, "\r\n this is a demo \r\n" );
 *            		 USART1_printf( USART1, "\r\n %d \r\n", i );
 *            		 USART1_printf( USART1, "\r\n %s \r\n", j );
 */
void USART1_printf(USART_TypeDef* pUSARTx, uint8_t *Data,...)
{
	const char *s;
	int d;
	char buf[16];

	va_list ap;//声明一个变量来转换参数列表
	va_start(ap, Data);//初始化变量

	while ( *Data != 0)     // 判断是否到达字符串结束符
	{
		if ( *Data == 0x5c )  //判断是否是'\'
		{
			switch ( *++Data )
			{
				case 'r':							          //回车符
					USART_SendData(pUSARTx, 0x0d);
					Data ++;
					break;

				case 'n':							          //换行符
					USART_SendData(pUSARTx, 0x0a);
					Data ++;
					break;

				default:
					Data ++;
					break;
			}
		}
		else if ( *Data == '%')
		{
			switch ( *++Data )
			{
				case 's':										  //字符串
					s = va_arg(ap, const char *);					//提取变参数
					for ( ; *s; s++)
					{
						USART_SendData(pUSARTx,*s);
						while( USART_GetFlagStatus(pUSARTx, USART_FLAG_TC) == RESET );
					}
					Data++;
					break;

				case 'd':										//十进制
					d = va_arg(ap, int);					//提取变参数
					itoa(d, buf, 10);						//将整形数据转换成字符串,用于打印显示
					for (s = buf; *s; s++)
					{
						USART_SendData(pUSARTx,*s);
						while( USART_GetFlagStatus(pUSARTx, USART_FLAG_TC) == RESET );
					}
					Data++;
					break;
				default:
					Data++;
					break;
			}
		} /* end of else if */
		else USART_SendData(pUSARTx, *Data++);
		while( USART_GetFlagStatus(pUSARTx, USART_FLAG_TC) == RESET );
	}
}

/*printf函数的重定向
调用：在syscalls.c中调用
使用：例如：printf("123\n");最后的\n不能缺少*/
int __io_putchar(int ch)
{
	while(USART_GetFlagStatus(USART1,USART_FLAG_TC)==RESET);
	USART_SendData(USART1,ch);
	return ch;
}

///*串口中断服务函数*/
//void USART1_IRQHandler(void)
//{
//	uint8_t uctemp;
//	if(USART_GetITStatus(USARTx,  USART_IT_RXNE) != RESET)
//	{
//		uctemp = USART_ReceiveData(USARTx);//用于数据接收
//		USART_SendByte(USARTx,uctemp);//用于数据转发
//
//	}
//}



