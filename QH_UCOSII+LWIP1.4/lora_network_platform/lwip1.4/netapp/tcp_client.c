/*
 * tcp_client.c
 *
 *  Created on: Jan 4, 2019
 *      Author: zm
 */
/*20.19.1.4 连接服务器ip地址：192.168.0.103，同一网段内，服务器端口是8080，避免与熟知端口冲突
 *          建立一个tcp客户端，回显服务器的数据
 *          功能：自动重新连接服务器,当连接断开或访问不到服务器时，一直尝试连接
 */

#include <netapp/tcp_client.h>
#include "lwip/tcp.h"
#include "lwip/ip_addr.h"


#if LWIP_TCP
//当连接失败后，调用echo_client_init重新建立客户端，重新开始连接
static void echo_client_conn_err(void *arg, err_t err)
{
	printf("connect error!,closed by core!\n");
	printf("try to connect to server again!\n");

	echo_client_init();
}
//当客户端向服务器发送数据成功后，这个函数被内核调用
static err_t echo_client_sent(void *arg, struct tcp_pcb *tpcb,  u16_t len)
{
	printf("echo client send data OK!seng len = [%d]\n",len);
	return ERR_OK;
}
//当接收到服务器的数据后，这个函数被调用
static err_t echo_client_recv(void *arg, struct tcp_pcb *pcb, struct pbuf *p, err_t err)
{
	if(p != NULL)
	{
		tcp_recved(pcb, p->tot_len);//更新接收窗口
		tcp_write(pcb,p->payload,p->len,1);//回显服务器的数据，将数据拷贝到内核再发送
		pbuf_free(p);
	}
	else if(err == ERR_OK)//如果服务器断开了连接
	{
		tcp_close(pcb);//先断开与服务器的连接
		echo_client_init();//再次建立新连接到服务器

		return ERR_OK;
	}

	return ERR_OK;
}
//当客户端同服务器连接建立成功后，这个函数被内核调用
static err_t echo_client_connected(void *arg, struct tcp_pcb *pcb, err_t err)
{
	char GRREETING[] = "HI,I am a new client.\n";

	tcp_recv(pcb, echo_client_recv);//注册recv函数
	tcp_sent(pcb, echo_client_sent);//注册sent函数

	tcp_write(pcb,GRREETING,sizeof(GRREETING),1);//向服务器发送GREETING
	return ERR_OK;
}



//客户端初始化函数，需要在内核初始化后调用一次
void echo_client_init(void)
{
	struct tcp_pcb * pcb = NULL;
	struct ip_addr server_ip;//定义服务器ip
	pcb = tcp_new();

	IP4_ADDR(&server_ip,192,168,0,103);//设置服务器ip地址:192,168,0,103
	tcp_connect(pcb,&server_ip,8080,echo_client_connected);//连接服务器8080端口，并注册connected回调函数
	tcp_err(pcb,echo_client_conn_err);//注册连接错误时调用的函数
	LWIP_DEBUGF(MY_DEBUG, ("echo_client_init\n"));
}
#endif




