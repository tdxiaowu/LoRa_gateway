/*
 * udp_demp.c
 *
 *  Created on: Dec 26, 2018
 *      Author: zm
 */

#include "netapp/udp_demo.h"
#include "lwip/udp.h"



/**
 * 回送程序实验：
 * 作为服务器，将本地接收到的UDP数据原样返回给源主机
 */
#define UDP_ECHO_PORT  7    //echo服务器熟知端口号，用于客户端访问

//编写用户ECHO程序回调函数
//当内核在连接upcb上收到数据后，会自动调用该函数，其中p为接收的数据包
static void udp_demo_callback(void *arg, struct udp_pcb *pcb, struct pbuf *p,ip_addr_t *addr, u16_t port)
{
	udp_sendto(pcb,p,addr,port);//直接将收到的数据返回到源端
	LWIP_DEBUGF(UDP_DEBUG, ("udp_demo send data.\n"));
	pbuf_free(p);//释放数据包空间
}

/**
 * ECHO程序初始化函数
 */
void udp_demo_init(void)
{
	struct udp_pcb *upcb;

	/*新建一个udp控制块*/
	upcb = udp_new();
	/*作为服务器，绑定到一个熟知端口上*/
	udp_bind(upcb, IP_ADDR_ANY, UDP_ECHO_PORT);
	/*注册回调函数*/
	udp_recv(upcb, udp_demo_callback,NULL);
	LWIP_DEBUGF(MY_DEBUG, ("udp_demo_init.\n"));
}



