/*
 * dhcp_thread.c
 *
 *  Created on: Mar 11, 2019
 *      Author: zm
 */
/*基于tcp的一个客户端用于返回dhcp请求后的ip地址
 * 需要连接路由器进行测试*/
#include "lwip/opt.h"
#include "lwip/api.h"
#include "lwip/dhcp.h"
#include "ucos_ii.h"
#include "lwip/api.h"
#include <stdio.h>



#if LWIP_DHCP
extern struct netif enc28j60_netif;
void dhcp_netconn_thread(void *arg)
{
	struct netconn *conn;
	struct ip_addr serveraddr;
	char sendbuf[20];
	u32_t err, wr_err;
	u16_t port = 8080;//端口号
	u16_t strlen = 0;
	int16_t count = 10;

	while(enc28j60_netif.dhcp->state != DHCP_BOUND)//等待ip地址有效
	{
		OSTimeDly(10);
	}

	IP4_ADDR(&serveraddr,192,168,0,103);//构造服务器ip地址

	//进程任务
	while(1)
	{
		conn = netconn_new(NETCONN_TCP);//申请一个ＴＣＰ连接结构
		err = netconn_connect(conn, &serveraddr, port);//连接服务器,端口号８０８０

		//是否连接成功
		if(err == ERR_OK)
		{
			printf("DHCP thread connect OK.\n");
			do{
				//构建包含网卡的ip地址的字符串
				strlen = sprintf(sendbuf,"DHCP--IP:%s\r\n",ipaddr_ntoa((ip_addr_t *)&(enc28j60_netif.ip_addr)));
				//发送到服务器
				wr_err = netconn_write(conn,sendbuf,strlen,NETCONN_NOCOPY);
				OSTimeDly(100);
			}while(count--);//循环发送10次
		}else
			printf("DHCP thread connect failed.\n");//连接失败

		netconn_close(conn);//关闭连接
		netconn_delete(conn);//删除连接结构
		OSTaskDel (OS_PRIO_SELF);//删除自己
	}
}
//创建dhcp客户端TCP进程
void dhcp_netconn_init()
{
	sys_thread_new("dhcp_netconnn_thread",dhcp_netconn_thread,NULL,DEFAULT_THREAD_STACKSIZE,TCPIP_THREAD_PRIO+1);
}
#endif
