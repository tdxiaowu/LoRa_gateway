/*
 * http_server.c
 *
 *  Created on: Jan 2, 2019
 *      Author: zm
 */
/*注意：测试是同网段的ip地址可以访问该页面
 * http服务器功能实现
 * 功能：提供固定网页html数据
 */
#include "netapp/http_server.h"
#include "lwip/tcp.h"
#include "lwip/memp.h"
#include "lwip/api.h"
#include "lwip/sys.h"
#include "lwip/dhcp.h"

#if LWIP_TCP
#define HTTP_PORT 80		//http服务器端口80
/*Raw接口的TCP HTTP*/
const unsigned char htmldata[] = "\
		<html>\
		<head><title>A lwip webserver!</title></head>\
			<center><p>A webserver based on lwip 1.4.1</center>\
		</html>";

//当服务器收到客户端的请求数据时，该函数被调用
/*
 *arg:传入的参数
 *tpcb:tcp控制块
 *p：接收到数据的指针
 */
static err_t http_recv(void *arg, struct tcp_pcb *pcb,struct pbuf *p, err_t err)
{
	char *data = NULL;

	if(p != NULL)
	{
		tcp_recved(pcb, p->tot_len);//更新接收窗口，len = p->tot_len
		data = p->payload;//data指向tcp数据区
		if(p->len >= 3 && data[0] == 'G' && data[1] == 'E' && data[2] == 'T')
		{
			tcp_write(pcb,htmldata,sizeof(htmldata),1);//向客户端返回html数据
		}
		else
		{
			printf("REQUEST  ERROR\n");
		}

		pbuf_free(p);//释放接收到数据包
		tcp_close(pcb);//关闭到客户端的连接
	}
	else//如果p = NULL，则客户端已关闭连接，服务器也需要关闭连接
	{
		if(err == ERR_OK)
		{
			return tcp_close(pcb);
		}
	}
	return ERR_OK;
}

//当有客户端建立连接后，处理函数
static err_t http_accept(void *arg, struct tcp_pcb *newpcb, err_t err)
{
	//配置lwip tcp使用的回调函数
	tcp_recv(newpcb,http_recv);//注册收到数据后处理函数
	return ERR_OK;
}


//http服务器初始化函数，调用一次即可，之后服务器一直会处于侦听状态，并接受来自任何客户端的连接
void http_server_init(void)
{
	err_t err;
	struct tcp_pcb * pcb = NULL;
	//新建一个控制块
	pcb = tcp_new();
	//绑定到http服务端口80
	err = tcp_bind(pcb,IP_ADDR_ANY,HTTP_PORT);

	if((err == ERR_USE) || (err == ERR_VAL))
	{
		memp_free(MEMP_TCP_PCB,pcb);//释放内存池
		printf("http_server tcp_bind err.\n");
		return ;
	}

	//服务器进入侦听状态
	pcb = tcp_listen(pcb);//

	//指定与客户端建立连接后要调用的函数
	tcp_accept(pcb,http_accept);
	LWIP_DEBUGF(MY_DEBUG, ("http_server_init\n"));

}
/*2019.3.18 增加服务器任务
 * 在静态ip地址下可以正常访问
 * 在动态ip地址下不能正常访问，出现硬件中断错误
 * */
/*带操作系统的Sequential API TCP http*/
/*web　服务器*/
/*HTML数据*/
extern struct netif enc28j60_netif;
const static char http_html_hdr[] = "HTTP/1.1 200 OK\r\tContent-type:text/html\r\n\r\n";
const static char http_index_html[] = "<!DOCTYPE html>\
<html>\
<head>\
	<title>Congrats!</title>\
</head>\
<body>\
<h1>Welcome to LwIP 1.4.1 HTTP server!</h1>\
<center><p>This is a test page base on netconn API.</p></center>\
</body>\
</html>";
//处理连接conn上的html请求
static void httpserver_server(struct netconn* conn)
{
	struct netbuf* inbuf;//接收数据指针
	char *buf;
	u16_t buflen;
	err_t err;


	err = netconn_recv(conn, &inbuf);//在连接上等待接收数据包

	if(err == ERR_OK)//成功接收数据
	{
		netbuf_data(inbuf, (void **)&buf, &buflen);//得到数据指针和数据长度
		//对数据进行处理
		if(buflen >= 5 && buf[0] == 'G' && buf[1] == 'E' && buf[2] == 'T' && buf[3] == ' ' && buf[4] == '/')
		{
			//若是GET请求，则向网页返回ＨＴＭＬ数据
			netconn_write(conn,http_html_hdr,sizeof(http_html_hdr)-1,NETCONN_NOCOPY);
			netconn_write(conn,http_index_html,sizeof(http_index_html)-1,NETCONN_NOCOPY);
		}
	}
	netconn_close(conn);//关闭连接
//	netconn_delete(conn);//接收的链接不能删除，只能关闭，自己建的pcb才能用此函数，这句造成，浏览器访问一次，协议栈报错了
	netbuf_delete(inbuf);//删除数据包

}
/*网页服务器主任务*/
static void httpserver_thread(void *arg)
{
	struct netconn* web_conn = NULL;
	struct netconn* newconn = NULL;
	err_t err;

#if LWIP_DHCP
	while(enc28j60_netif.dhcp->state != DHCP_BOUND)//等待ip地址有效
	{
		OSTimeDly(10);
	}
#endif
	web_conn = netconn_new(NETCONN_TCP);	//新建一个TCP连接
	netconn_bind(web_conn, IP_ADDR_ANY, 80);	//与本地的http端口80绑定


	netconn_listen(web_conn);/*进入侦听模式*/

	do{
		err = netconn_accept(web_conn,&newconn);//阻塞，直到接收到客户端的新连接

		if(err == ERR_OK)//连接有效
		{
			httpserver_server(newconn);//处理连接上的数据
			netconn_delete(newconn);//删除新的连接空间
		}

	}while(err == ERR_OK);//连接错误时退出
	printf("http_server netconn_accpte failed!\r\n");
	/*服务器接收连接失败，则清除服务器资源*/
	netconn_close(web_conn);
	netconn_delete(web_conn);
}

void httpserver_create(void)
{
	sys_thread_new("http_server_netconn", httpserver_thread , NULL, DEFAULT_THREAD_STACKSIZE, TCPIP_THREAD_PRIO + 1);
}

#endif
