/*
 * Lwipopts.h
 *
 *  Created on: Apr 20, 2018
 *      Author: zm
 */

#ifndef __LWIPOPTS_H_
#define __LWIPOPTS_H_


#define NO_SYS                       0//１：无操作系统模拟层；０：有操作系统模拟层

#define SYS_LIGHTWEIGHT_PROT		1//临界中断保护开关（多任务模式下开启）

#define LWIP_SOCKET  				(NO_SYS == 0)//0:不编译socket API 的相关文件,当使用操作系统时改为１
#define LWIP_NETCONN 				(NO_SYS == 0) //0:不编译sequential API 的相关文件,当使用操作系统时改为１
/*协议栈配置*/
#define MEM_ALIGNMENT                4			//内存字节对齐方式，与平台密切相关
#define MEM_SIZE                     1024*10   //定义内存堆大小
#define TCP_MSS                      (1500-40)  //TCP报文段最大长度
#define TCP_SND_BUF                  (2*TCP_MSS)       //允许TCP协议使用的最大发送缓冲长度
#define ETH_PAD_SIZE				 0		//以太网首部填充
#define MEMP_NUM_TCPIP_MSG_INPKT        100//接收tcpip信息的个数
#define TCP_OVERSIZE                    0//禁用tcp超大分配

/*模块开关*/
#define LWIP_UDP                        1	//打开UDP功能模块
#define LWIP_DHCP                       0	//打开DHCP功能模块，使用dhcp必须打开udp
#define LWIP_DNS                        0	//打开DNS功能模块
#define LWIP_TCP                        0	//打开TCP功能模块

#define PPPOE_SUPPORT                   0



/*协议栈进程*/
#define TCPIP_THREAD_PRIO               1
/*调试总开关*/
#define LWIP_DBG_TYPES_ON           LWIP_DBG_ON
/***********************************************/
/*各模块调试开关*/
#define NETIF_DEBUG                 LWIP_DBG_OFF	//允许程序打印网络接口调试信息
#define ETHARP_DEBUG                LWIP_DBG_OFF  	//允许程序打印ARP调试信息
#define ICMP_DEBUG                  LWIP_DBG_OFF 	//允许程序打印相关ICMP调试信息
#define IP_DEBUG                    LWIP_DBG_OFF	//允许程序打印相关IP调试信息
#define RAW_DEBUG                   LWIP_DBG_OFF		//允许程序打印相关RAW调试信息
#define UDP_DEBUG                   LWIP_DBG_OFF	//允许程序打印相关UDP调试信息
#define TCP_DEBUG                   LWIP_DBG_OFF
#define TCPIP_DEBUG					LWIP_DBG_OFF	//允许程序打印相关tcpip进程的调试信息
#define DHCP_DEBUG					LWIP_DBG_OFF		//允许程序打印相关dhcp的调试信息

//自定义的调试信息开关
#define PING_DEBUG                  LWIP_DBG_ON    //ping调试信息开关
#define MY_DEBUG                    LWIP_DBG_ON    //自己定义的调试信息

#endif /* ARCH_LWIPOPTS_H_ */
