
#include "netapp/lwipconfig.h"
#include "lwip/netif.h"
#include "lwip/ip.h"
#include "lwip/tcp.h"
#include "lwip/init.h"
#include "netif/etharp.h"
#include "lwip/timers.h"
#include "netapp/ping.h"
#include "netapp/udp_demo.h"
#include "netapp/http_server.h"
#include "netapp/tcp_client.h"
#include "lwip/tcpip.h"
#include "lwip/dhcp.h"
#include "ucos_ii.h"
//extern functions
extern err_t ethernetif_init(struct netif *netif);//以太网接口初始化函数
void tcpserver_init(void);

#if LWIP_DHCP
void lwip_comm_dhcp_creat(void);//创建dhcp任务
#define LWIP_DHCP_TASK_PRIO  4//定义dhcp任务优先级
#define LWIP_MAX_DHCP_TRIES  5//定义dhcp最大尝试次数
#define LWIP_DHCP_STK_SIZE   512//定义dhcp任务堆栈大小
OS_STK LWIP_DHCP_TASK_STK[LWIP_DHCP_STK_SIZE];//定义dhcp任务堆栈
#endif
//定义全局网络接口变量
struct netif enc28j60_netif;

/*显示地址函数*/
void show_address(void)
{

		printf("ip     :%s.\r\n",ipaddr_ntoa((ip_addr_t *)&(enc28j60_netif.ip_addr)));
		printf("gw     :%s.\r\n",ipaddr_ntoa((ip_addr_t *)&(enc28j60_netif.gw)));
		printf("netmask:%s.\r\n",ipaddr_ntoa((ip_addr_t *)&(enc28j60_netif.netmask)));

}

/*lwip初始化函数*/
void lwip_init_task(void)
{
	/*定义网络接口需要IP地址的结构*/
	struct ip_addr ipaddr,//新建IP
				   netmask,//子网掩码
				   gw;//网关地址

	/*初始化协议栈*/
#if NO_SYS
    lwip_init();//协议栈初始化函数

#else
    tcpip_init(NULL,NULL);//该函数调用lwip_init完成内核初始化，同时创建内核进程
#endif//NO_SYS

    /*初始化网络接口IP地址*/
#if LWIP_DHCP
    /*若使用的DHCP，各部分IP地址设置为０*/
    IP4_ADDR(&ipaddr,0,0,0,0);//设置网卡ip地址
	IP4_ADDR(&netmask,0,0,0,0);//设置网络掩码
	IP4_ADDR(&gw,0,0,0,0);//设置网络接口网关地址
#else
    /*若未使用DHCP,初始化各部分IP地址*/
    IP4_ADDR(&ipaddr,192,168,0,18);//设置网卡ip地址
	IP4_ADDR(&netmask, 255,255,255,0);//设置网络掩码
	IP4_ADDR(&gw, 192,168,0,1);//设置网络接口网关地址
#endif/*LWIP_DHCP*/

/*向协议栈中添加一个网络接口*/
#if NO_SYS
    netif_add(&enc28j60_netif, &ipaddr, &netmask, &gw, NULL, ethernetif_init,ethernet_input);

#else/*有操作系统*/
    netif_add(&enc28j60_netif, &ipaddr, &netmask, &gw, NULL, ethernetif_init,tcpip_input);
#endif/*NO_SYS*/

    /*启用网络接口*/
	netif_set_default(&enc28j60_netif);//设置协议栈默认的网络接口
	netif_set_up(&enc28j60_netif);//启用协议栈中一个网络接口
	show_address();//打印网口的ip地址
#if NO_SYS
#if LWIP_DHCP
	dhcp_start(&enc28j60_netif);//启动DHCP，向DHCP服务器请求ip地址
#endif
#else
#if LWIP_DHCP
	lwip_comm_dhcp_creat();//创建DHCP任务
#endif
#endif

}

#if LWIP_DHCP
static void lwip_comm_dhcp_delete(void)
{
//	dhcp_stop(&enc28j60_netif); 		//停止dhcp
	OSTaskDel(LWIP_DHCP_TASK_PRIO);	//删除DHCP任务
}
//DHCP处理任务
static void lwip_dhcp_task(void *pdata)
{
    u32 ip=0,netmask=0,gw=0;
    dhcp_start(&enc28j60_netif);//开启DHCP
    printf("start search DHCP server,please wait...........\r\n");
    while(1)
    {
        printf("searching address...\r\n");
        ip=enc28j60_netif.ip_addr.addr;        //读取新IP地址
        netmask=enc28j60_netif.netmask.addr;//读取子网掩码
        gw=enc28j60_netif.gw.addr;            //读取默认网关
        if(ip!=0)                       //当正确读取到IP地址的时候
        {
        	printf("searched address:\r\n");
        	show_address();
            break;
        }else if(enc28j60_netif.dhcp->tries>LWIP_MAX_DHCP_TRIES) //通过DHCP服务获取IP地址失败,且超过最大尝试次数
        {
            printf("dhcp failed\r\n");
            printf("use static address\r\n");
            IP4_ADDR(&(enc28j60_netif.ip_addr),192,168,0,18);//设置网卡ip地址
        	IP4_ADDR(&(enc28j60_netif.netmask), 255,255,255,0);//设置网络掩码
        	IP4_ADDR(&(enc28j60_netif.gw), 192,168,0,1);//设置网络接口网关地址
        	show_address();
            break;
        }
        OSTimeDly(50); //延时
    }
    lwip_comm_dhcp_delete();//删除DHCP任务

}
//创建DHCP任务
void lwip_comm_dhcp_creat(void)
{
    OS_CPU_SR cpu_sr;
    OS_ENTER_CRITICAL();  //进入临界区
    /*创建DHCP任务*/
    OSTaskCreate(lwip_dhcp_task,
    			(void*)0,
    			(OS_STK*)&LWIP_DHCP_TASK_STK[LWIP_DHCP_STK_SIZE-1],
				LWIP_DHCP_TASK_PRIO);

    OS_EXIT_CRITICAL();  //退出临界区
}
#endif

#if NO_SYS
extern void process_mac(void);
/**/
void lwip_task(void *pdata)
{

	//init LwIP
	lwip_init_task();//协议栈初始化


	//init a tcp server
 	//tcpserver_init();

 	/*ping功能初始化设置*/
 	//ping_init();

 	/*udp通信实例初始化设置*/
// 	udp_demo_init();

 	/*HTTP服务器初始化*/
// 	http_server_init();

 	/*tcp客户端初始化*/
// 	echo_client_init();


	//for periodic handle
	while(1)
	{
		process_mac();
		
		//process LwIP timeout
		sys_check_timeouts();//检查各定时事件的超时时间，处理超时的事件
		
		//todo: add your own user code here

	}



}
#endif
void tcp_send_pbuf_data_out(struct tcp_pcb *pcb, struct pbuf *p)
{
	if (NULL == pcb || NULL == p)
	{
	    return;
	}
		
	while(p != NULL)
	{
		tcp_write(pcb, p->payload, p->len, 0);
		p = p->next;
	}
}

static err_t http_recv(void *arg, struct tcp_pcb *pcb, struct pbuf *p, err_t err)
{

  //  char *rq;

  /* We perform here any necessary processing on the pbuf */
  if (p != NULL) 
  {        
	/* We call this function to tell the LwIp that we have processed the data */
	/* This lets the stack advertise a larger window, so more data can be received*/
	tcp_recved(pcb, p->tot_len);

     {
      // Uart_Printf("Do tcp write when receive\n");
       tcp_write(pcb, p->payload, p->len, 0);
	   tcp_send_pbuf_data_out(pcb,p);
	   //tcp_write(pcb, p->payload, p->len, 0);
     }
     pbuf_free(p);
    } 
  else if (err == ERR_OK) 
  {
    /* When the pbuf is NULL and the err is ERR_OK, the remote end is closing the connection. */
    /* We free the allocated memory and we close the connection */
    return tcp_close(pcb);
  }
  return ERR_OK;
}

/**
  * @brief  This function when the Telnet connection is established
  * @param  arg  user supplied argument 
  * @param  pcb	 the tcp_pcb which accepted the connection
  * @param  err	 error value
  * @retval ERR_OK
  */

static err_t http_accept(void *arg, struct tcp_pcb *pcb, err_t err)
{     
  
  /* Tell LwIP to associate this structure with this connection. */
 // tcp_arg(pcb, mem_calloc(sizeof(struct name), 1));	
  
  /* Configure LwIP to use our call back functions. */
 // tcp_err(pcb, HelloWorld_conn_err);
 // tcp_setprio(pcb, TCP_PRIO_MIN);
  tcp_recv(pcb, http_recv);
 // tcp_poll(pcb, http_poll, 10);
 //  tcp_sent(pcb, http_sent);
  /* Send out the first message */  
 // tcp_write(pcb, GREETING, strlen(GREETING), 1); 
  return ERR_OK;
}

/**
  * @brief  Initialize the hello application  
  * @param  None 
  * @retval None 
  */
 
void tcpserver_init(void)
{
  struct tcp_pcb *pcb;	            		
  
  /* Create a new TCP control block  */
  pcb = tcp_new();	                		 	

  /* Assign to the new pcb a local IP address and a port number */
  /* Using IP_ADDR_ANY allow the pcb to be used by any local interface */
  tcp_bind(pcb, IP_ADDR_ANY, 6060);       


  /* Set the connection to the LISTEN state */
  pcb = tcp_listen(pcb);				

  /* Specify the function to be called when a connection is established */	
  tcp_accept(pcb, http_accept);   
										
}
