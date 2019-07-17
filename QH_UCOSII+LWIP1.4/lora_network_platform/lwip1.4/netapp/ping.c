/*
 * ping.c
 *
 *  Created on: Dec 21, 2018
 *      Author: zm
 */
/*实现平台自主定时向目的主机发送ping请求，以确保与目的主机通信正常
 * 函数实现打印发送请求序列
 * 打印应答时间
 *
 * 可添加收到标志用于其他应用使用
 * 2018.12.27 增加静态全局变量ping_state，在系统接收到回送应答后，设置ping_state为１，不再进行ping_timeout函数注册
 *
 * */
#include "netapp/ping.h"

#include "lwip/opt.h"
#include "lwip/raw.h"
#include "lwip/ip_addr.h"
#include "lwip/ip.h"
#include "lwip/timers.h"
#include "lwip/pbuf.h"
#include "lwip/icmp.h"
#include "lwip/inet_chksum.h"

extern u32_t sys_now(void);
/*宏定义*/
#define  PING_DELAY    1000    //ping请求间隔
#define  PING_ID       0xAFAF
#define  PING_DATA_SIZE    32  //ping包携带的数据长度

/*全局变量*/
static struct raw_pcb * ping_pcb;
static u16_t ping_seq_num;
static u32_t ping_time;
static ip_addr_t ping_dst;
static volatile u8_t ping_state;


//该函数被注册到ping_pcb原始控制块的recv字段，当内核接收到icmp回送应答包数据包时，会调用该函数处理
static u8_t ping_raw_recv_fn(void *arg, struct raw_pcb *pcb, struct pbuf *p,ip_addr_t *des)
{
	struct icmp_echo_hdr *iechohdr;

	if(p->tot_len >= (PBUF_IP_HLEN + sizeof(struct icmp_echo_hdr)))
	{
		iechohdr = (struct icmp_echo_hdr *)((u8_t*)p->payload + PBUF_IP_HLEN);
		//判断类型是回送应答包才打印信息
		if((iechohdr->type == ICMP_ER) && (iechohdr->id == PING_ID)&& (iechohdr->seqno = htons(ping_seq_num)))
		{

			ping_state = 1;//ping通过
			//打印ping相应的相关信息到串口
			LWIP_DEBUGF(PING_DEBUG, ("ping:recv[%"U32_F"]",ntohs(iechohdr->seqno)));
			ip_addr_debug_print(PING_DEBUG, des);//打印目的地址
			LWIP_DEBUGF(PING_DEBUG, ("time = [%"U32_F"]ms",(sys_now() - ping_time)));
			LWIP_DEBUGF(PING_DEBUG, ("\n"));
			LWIP_DEBUGF(PING_DEBUG, ("ping success!\n"));

				//释放数据包
			pbuf_free(p);
			return 1;
		}


	}

	return 0;
}
//完成ICMP数据包的填写
static void ping_prepare_echo(struct icmp_echo_hdr *iechohdr, u16_t len)
{
	u16_t i;
	u16_t data_len = len - sizeof(struct icmp_echo_hdr);

	//填写icmp请求首部
	ICMPH_TYPE_SET(iechohdr,ICMP_ECHO);//填写类型
	ICMPH_CODE_SET(iechohdr,0);//填写代码
	iechohdr->chksum = 0;
	iechohdr->id = PING_ID;
	iechohdr->seqno = htons(++ping_seq_num);
//数据区填充数据
	for(i = 0; i < data_len; i++)
	{
		((char*)iechohdr)[sizeof(struct icmp_echo_hdr) + i] = (char)i;

	}
//计算校验和
	iechohdr->chksum = inet_chksum(iechohdr,len);
}

//向目的ip,发送ping回送请求包
static void ping_send(struct raw_pcb *pcb, ip_addr_t *des)
{
	struct pbuf *p;//定义一个数据包，用来新建icmp回送请求包
	struct icmp_echo_hdr *iechohdr;
	u16_t ping_size = sizeof(struct icmp_echo_hdr) + PING_DATA_SIZE;
	//为icmp回送请求包申请包空间
	p = pbuf_alloc(PBUF_IP, ping_size, PBUF_RAM);
	//检查是否申请成功
	if(p == NULL)
	{
		return;
	}

	//申请成功
	if((p->len == p->tot_len) && (p->next == NULL))
	{
		iechohdr = (struct icmp_echo_hdr *)p->payload;//iechohdr指向申请的空间中的ICMP首部
		ping_prepare_echo(iechohdr,ping_size);//填写ICMP数据包

		raw_sendto(pcb,p,des);//发送数据ICMP数据包

		ping_time = sys_now();//记录发送时系统时间，以便计算往返时间
		//打印ping请求的相关信息到串口
		 LWIP_DEBUGF(PING_DEBUG, ("ping:[%"U32_F"]send",ping_seq_num));
		 ip_addr_debug_print(PING_DEBUG, des);//打印目的地址
		 LWIP_DEBUGF(PING_DEBUG, ("\n"));
	}
	//发送完成，删除数据包空间
	pbuf_free(p);

}

//ping超时,向目的ip发送ping回送请求函数
static void ping_timeout(void *arg)
{
	struct raw_pcb * pcb = (struct raw_pcb *)arg;//获得ping_pcb

	//如果没有ping通,再次发送
	if(ping_state == 0)
	{
		//发送ping回送请求包
		ping_send(pcb, &ping_dst);
		//注册下一个定时事件，PING_DELAY时间后，ping_timeout再次被内核调用
		sys_timeout(PING_DELAY, ping_timeout, pcb);
	}
	else//ping成功后，删除ping控制块
		raw_remove(ping_pcb);//删除ping_pcb协议控制块

}

//初始化ping原始协议控制块
static void ping_raw_init(void)
{
	//申请一个ICMP类型的协议控制块
	ping_pcb = raw_new(IP_PROTO_ICMP);
	//ping_pcb中注册recv回调函数
	raw_recv(ping_pcb, ping_raw_recv_fn, NULL);
	//向ping_pcb中写入本地ip地址，这里写成全０
	raw_bind(ping_pcb, IP_ADDR_ANY);
	//设置一个定时事件，在PING_DELAY时间后，ping_timeout函数将被内核调用，回调参数为ping_pcb
	sys_timeout(PING_DELAY, ping_timeout, ping_pcb);
}


//ping应用程序初始化函数119.75.217.109　　172.20.199.194
//在内核初始化完成后，调用该函数，ping程序就能开始工作了
void ping_init(void)
{
	IP4_ADDR(&ping_dst, 172,20,199,194);				//初始化目的ip,ping_dst
	ping_raw_init();								//初始化ping原始协议控制块
	ping_state = 0;//设置ping初始状态
}
//ping初始化，输入陪ip地址字符串
void ping_ip(const char *pip)
{
	ipaddr_aton(pip, &ping_dst);//将ip地址字符串填入到目的ip指针上
	ping_raw_init();								//初始化ping原始协议控制块
	ping_state = 0;//设置ping初始状态
}






