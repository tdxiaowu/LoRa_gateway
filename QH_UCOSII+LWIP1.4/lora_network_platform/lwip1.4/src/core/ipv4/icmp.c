/**
 * @file
 * ICMP - Internet Control Message Protocol
 *
 */

/*
 * Copyright (c) 2001-2004 Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 *
 * Author: Adam Dunkels <adam@sics.se>
 *
 */

/* Some ICMP messages should be passed to the transport protocols. This
   is not implemented. */

#include "lwip/opt.h"

#if LWIP_ICMP /* don't build if not configured for use in lwipopts.h */

#include "lwip/icmp.h"
#include "lwip/inet_chksum.h"
#include "lwip/ip.h"
#include "lwip/def.h"
#include "lwip/stats.h"
#include "lwip/snmp.h"

#include <string.h>

/** Small optimization: set to 0 if incoming PBUF_POOL pbuf always can be
 * used to modify and send a response packet (and to 1 if this is not the case,
 * e.g. when link header is stripped of when receiving) */
#ifndef LWIP_ICMP_ECHO_CHECK_INPUT_PBUF_LEN
#define LWIP_ICMP_ECHO_CHECK_INPUT_PBUF_LEN 1
#endif /* LWIP_ICMP_ECHO_CHECK_INPUT_PBUF_LEN */

/* The amount of data from the original packet to return in a dest-unreachable */
#define ICMP_DEST_UNREACH_DATASIZE 8//定义宏，引起差错的IP数据报数据区将被差错报文装载的长度

static void icmp_send_response(struct pbuf *p, u8_t type, u8_t code);

/**处理协议栈收到的ICMP报文，在ip_input中被调用
 * Processes ICMP input packets, called from ip_input().
 *
 * Currently only processes icmp echo requests and sends
 * out the echo response.
 *
 * @param p the icmp echo request packet, p->payload pointing to the ip header
 * @param inp the netif on which this packet was received
 */
void
icmp_input(struct pbuf *p, struct netif *inp)
{
  u8_t type;
#ifdef LWIP_DEBUG
  u8_t code;
#endif /* LWIP_DEBUG */
  struct icmp_echo_hdr *iecho;//定义一个icmp报文首部指针
  struct ip_hdr *iphdr;//定义一个ip数据报首部指针
  s16_t hlen;

  ICMP_STATS_INC(icmp.recv);
  snmp_inc_icmpinmsgs();


  iphdr = (struct ip_hdr *)p->payload;//指向接收到的IP数据报首部
  hlen = IPH_HL(iphdr) * 4;//计算ip数据报首部长度
  //调整pbuf的payload指针，使其指向icmp报文首部
  if (pbuf_header(p, -hlen) || (p->tot_len < sizeof(u16_t)*2)) {
    LWIP_DEBUGF(ICMP_DEBUG, ("icmp_input: short ICMP (%"U16_F" bytes) received\n", p->tot_len));
    goto lenerr;
  }

  type = *((u8_t *)p->payload);//获得icmp首部中类型字段
#ifdef LWIP_DEBUG
  code = *(((u8_t *)p->payload)+1);//获得icmp首部中代码字段
#endif /* LWIP_DEBUG */
  //根据不同类型做出不同处理
  switch (type) {
  case ICMP_ER://回送请求的应答，忽略
    /* This is OK, echo reply might have been parsed by a raw PCB
       (as obviously, an echo request has been sent, too). */
    break; 
  case ICMP_ECHO://回送请求
	  //首先检查报文的目的地址是否合法
#if !LWIP_MULTICAST_PING || !LWIP_BROADCAST_PING
    {
      int accepted = 1;//局部变量，标志是否对ICMP回送请求进行回应
#if !LWIP_MULTICAST_PING
      /* multicast destination address?如果是多播，不回应 */
      if (ip_addr_ismulticast(&current_iphdr_dest)) {
        accepted = 0;
      }
#endif /* LWIP_MULTICAST_PING */
#if !LWIP_BROADCAST_PING
      /* broadcast destination address? 如果是广播，不回应*/
      if (ip_addr_isbroadcast(&current_iphdr_dest, inp)) {
        accepted = 0;
      }
#endif /* LWIP_BROADCAST_PING */
      /* broadcast or multicast destination address not acceptd? */
      if (!accepted) {
        LWIP_DEBUGF(ICMP_DEBUG, ("icmp_input: Not echoing to multicast or broadcast pings\n"));
        ICMP_STATS_INC(icmp.err);
        pbuf_free(p);//释放接收到的报文
        return;
      }
    }
#endif /* !LWIP_MULTICAST_PING || !LWIP_BROADCAST_PING */
    //检查报文长度是否合法
    LWIP_DEBUGF(ICMP_DEBUG, ("icmp_input: ping\n"));
    if (p->tot_len < sizeof(struct icmp_echo_hdr)) {//如果报文长度比icmp首部还小
      LWIP_DEBUGF(ICMP_DEBUG, ("icmp_input: bad ICMP echo received\n"));
      goto lenerr;
    }
    //判断校验和是否正确
    if (inet_chksum_pbuf(p) != 0) {//校验和不对
      LWIP_DEBUGF(ICMP_DEBUG, ("icmp_input: checksum failed for received ICMP echo\n"));
      pbuf_free(p);//释放接收到的报文
      ICMP_STATS_INC(icmp.chkerr);
      snmp_inc_icmpinerrors();
      return;
    }
#if LWIP_ICMP_ECHO_CHECK_INPUT_PBUF_LEN
    if (pbuf_header(p, (PBUF_IP_HLEN + PBUF_LINK_HLEN))) {
      /* p is not big enough to contain link headers
       * allocate a new one and copy p into it
       */
      struct pbuf *r;
      /* switch p->payload to ip header */
      if (pbuf_header(p, hlen)) {
        LWIP_ASSERT("icmp_input: moving p->payload to ip header failed\n", 0);
        goto memerr;
      }
      /* allocate new packet buffer with space for link headers */
      r = pbuf_alloc(PBUF_LINK, p->tot_len, PBUF_RAM);
      if (r == NULL) {
        LWIP_DEBUGF(ICMP_DEBUG, ("icmp_input: allocating new pbuf failed\n"));
        goto memerr;
      }
      LWIP_ASSERT("check that first pbuf can hold struct the ICMP header",
                  (r->len >= hlen + sizeof(struct icmp_echo_hdr)));
      /* copy the whole packet including ip header */
      if (pbuf_copy(r, p) != ERR_OK) {
        LWIP_ASSERT("icmp_input: copying to new pbuf failed\n", 0);
        goto memerr;
      }
      iphdr = (struct ip_hdr *)r->payload;
      /* switch r->payload back to icmp header */
      if (pbuf_header(r, -hlen)) {
        LWIP_ASSERT("icmp_input: restoring original p->payload failed\n", 0);
        goto memerr;
      }
      /* free the original p */
      pbuf_free(p);
      /* we now have an identical copy of p that has room for link headers */
      p = r;
    } else {
      /* restore p->payload to point to icmp header */
      if (pbuf_header(p, -(s16_t)(PBUF_IP_HLEN + PBUF_LINK_HLEN))) {
        LWIP_ASSERT("icmp_input: restoring original p->payload failed\n", 0);
        goto memerr;
      }
    }
#endif /* LWIP_ICMP_ECHO_CHECK_INPUT_PBUF_LEN */
    //到这，所有的校验工作都完成，调整回送请求报文中的相关字段生成回送回答报文
    //交换数据报中的源和目的ip地址，填写报文类型
    /* At this point, all checks are OK. */
    /* We generate an answer by switching the dest and src ip addresses,
     * setting the icmp type to ECHO_RESPONSE and updating the checksum. */
    iecho = (struct icmp_echo_hdr *)p->payload;//指向icmp请求报文首部
    ip_addr_copy(iphdr->src, *ip_current_dest_addr());//填写数据报中的源ip地址
    ip_addr_copy(iphdr->dest, *ip_current_src_addr());//填写数据报中的目的ip地址
    ICMPH_TYPE_SET(iecho, ICMP_ER);//填写icmp类型为回送回答
#if CHECKSUM_GEN_ICMP
    /* adjust the checksum */
    //填写报文的校验和字段，因为回送回答和回送请求，只有报文首部类型变了，适当调整原来的校验和即可
    if (iecho->chksum >= PP_HTONS(0xffffU - (ICMP_ECHO << 8))) {//有进位
      iecho->chksum += PP_HTONS(ICMP_ECHO << 8) + 1;
    } else {//没有进位
      iecho->chksum += PP_HTONS(ICMP_ECHO << 8);
    }
#else /* CHECKSUM_GEN_ICMP */
    iecho->chksum = 0;
#endif /* CHECKSUM_GEN_ICMP */

    /* Set the correct TTL and recalculate the header checksum. */
    IPH_TTL_SET(iphdr, ICMP_TTL);//设置ip数据报中的ＴＴＬ字段
    IPH_CHKSUM_SET(iphdr, 0);//ip首部校验和清零
#if CHECKSUM_GEN_IP
    IPH_CHKSUM_SET(iphdr, inet_chksum(iphdr, IP_HLEN));//计算并填写ip首部校验和
#endif /* CHECKSUM_GEN_IP */

    ICMP_STATS_INC(icmp.xmit);
    /* increase number of messages attempted to send */
    snmp_inc_icmpoutmsgs();
    /* increase number of echo replies attempted to send */
    snmp_inc_icmpoutechoreps();
//在将报文递交给ip层发送前需要先将pbuf的payload指针指向ip首部
    if(pbuf_header(p, hlen)) {//调整pbuf的payload指针指向ip首部
      LWIP_ASSERT("Can't move over header in packet", 0);
    } else {
      err_t ret;
      //调用ip_output_if直接发送，并设置IP_HDRINCL，表示ip首部已经组装好
      /* send an ICMP packet, src addr is the dest addr of the curren packet */
/**自己添加的***/
      LWIP_DEBUGF(MY_DEBUG, ("icmp_input: ping reply start\n"));

      ret = ip_output_if(p, ip_current_dest_addr(), IP_HDRINCL,
                   ICMP_TTL, 0, IP_PROTO_ICMP, inp);
/**自己添加的***/
      LWIP_DEBUGF(MY_DEBUG, ("icmp_input: ping reply end\n"));//自己添加的调试信息

      if (ret != ERR_OK) {
        LWIP_DEBUGF(ICMP_DEBUG, ("icmp_input: ip_output_if returned an error: %c.\n", ret));
      }
    }
    break;
  default://其他类型的icmp报文，不做任何处理，删除后直接返回
    LWIP_DEBUGF(ICMP_DEBUG, ("icmp_input: ICMP type %"S16_F" code %"S16_F" not supported.\n", 
                (s16_t)type, (s16_t)code));
    ICMP_STATS_INC(icmp.proterr);
    ICMP_STATS_INC(icmp.drop);
  }
  pbuf_free(p);
  return;
lenerr:
  pbuf_free(p);
  ICMP_STATS_INC(icmp.lenerr);
  snmp_inc_icmpinerrors();
  return;
#if LWIP_ICMP_ECHO_CHECK_INPUT_PBUF_LEN
memerr:
  pbuf_free(p);
  ICMP_STATS_INC(icmp.err);
  snmp_inc_icmpinerrors();
  return;
#endif /* LWIP_ICMP_ECHO_CHECK_INPUT_PBUF_LEN */
}

/**发送一个目的站不可达差错报文，被ip_input函数调用
 * Send an icmp 'destination unreachable' packet, called from ip_input() if
 * the transport layer protocol is unknown and from udp_input() if the local
 * port is not bound.
 *
 * @param p the input packet for which the 'unreachable' should be sent,
 *          p->payload pointing to the IP header
 * @param t type of the 'unreachable' packet
 */
void
icmp_dest_unreach(struct pbuf *p, enum icmp_dur_type t)
{
  icmp_send_response(p, ICMP_DUR, t);
}

#if IP_FORWARD || IP_REASSEMBLY
/**发送一个数据报超时差错报文，被ip_forward函数调用
 * Send a 'time exceeded' packet, called from ip_forward() if TTL is 0.
 *
 * @param p the input packet for which the 'time exceeded' should be sent,
 *          p->payload pointing to the IP header
 * @param t type of the 'time exceeded' packet
 */
void
icmp_time_exceeded(struct pbuf *p, enum icmp_te_type t)
{
  icmp_send_response(p, ICMP_TE, t);
}

#endif /* IP_FORWARD || IP_REASSEMBLY */

/**发送一个ICMP差错报文
 * Send an icmp packet in response to an incoming packet.
 *
 * @param p the input packet for which the 'unreachable' should be sent,
 *          p->payload pointing to the IP header
 * @param type Type of the ICMP header
 * @param code Code of the ICMP header
 */
static void
icmp_send_response(struct pbuf *p, u8_t type, u8_t code)
{
  struct pbuf *q;//定义一个新的数据包结构体指针
  struct ip_hdr *iphdr;//定义一个新的ip数据包首部结构体指针
  /* we can use the echo header here */
  //利用回送请求结构体指针描述差错报文的首部
  struct icmp_echo_hdr *icmphdr;//定义一个差错报文首部指针
  ip_addr_t iphdr_src;//用于记录源ip地址的变量

  /**ICMP属于网络层，申请一个网络层上的数据包，pbuf中预留IP首部和以太网首部空间，
  pbuf的数据区长度为差错报文首部长度＋差错报文数据长度（IP首部长度＋８）*/
  /* ICMP header + IP header + 8 bytes of data */
  q = pbuf_alloc(PBUF_IP, sizeof(struct icmp_echo_hdr) + IP_HLEN + ICMP_DEST_UNREACH_DATASIZE,
                 PBUF_RAM);
  if (q == NULL) {//如果申请数据包空间失败，直接返回
    LWIP_DEBUGF(ICMP_DEBUG, ("icmp_time_exceeded: failed to allocate pbuf for ICMP packet.\n"));
    return;
  }
  LWIP_ASSERT("check that first pbuf can hold icmp message",
             (q->len >= (sizeof(struct icmp_echo_hdr) + IP_HLEN + ICMP_DEST_UNREACH_DATASIZE)));

  iphdr = (struct ip_hdr *)p->payload;//指向引起差错的IP数据报的首部
  LWIP_DEBUGF(ICMP_DEBUG, ("icmp_time_exceeded from "));
  ip_addr_debug_print(ICMP_DEBUG, &(iphdr->src));
  LWIP_DEBUGF(ICMP_DEBUG, (" to "));
  ip_addr_debug_print(ICMP_DEBUG, &(iphdr->dest));
  LWIP_DEBUGF(ICMP_DEBUG, ("\n"));

  icmphdr = (struct icmp_echo_hdr *)q->payload;//指向申请数据包的数据区，即差错报文的首部
  icmphdr->type = type;//填写类型字段
  icmphdr->code = code;//填写代码字段
  //对于目的不可达和数据报超时报文，首部剩下的４个字节都为０
  icmphdr->id = 0;
  icmphdr->seqno = 0;

  //将引起差错的IP数据报的IP首部＋８字节数据拷贝到差错报文的数据区域
  /* copy fields from original packet */
  SMEMCPY((u8_t *)q->payload + sizeof(struct icmp_echo_hdr), (u8_t *)p->payload,
          IP_HLEN + ICMP_DEST_UNREACH_DATASIZE);
//计算校验和并填写
  /* calculate checksum */
  icmphdr->chksum = 0;//先将报文中的校验和字段清零
  icmphdr->chksum = inet_chksum(icmphdr, q->len);//计算并填写校验和
  ICMP_STATS_INC(icmp.xmit);
  /* increase number of messages attempted to send */
  snmp_inc_icmpoutmsgs();
  /* increase number of destination unreachable messages attempted to send */
  snmp_inc_icmpouttimeexcds();
  ip_addr_copy(iphdr_src, iphdr->src);//获得源ip地址，在发送时差错报文时用作目的IP地址
  ip_output(q, NULL, &iphdr_src, ICMP_TTL, 0, IP_PROTO_ICMP);//调用IP层函数输出ICMP报文
  pbuf_free(q);//释放ICMP报文占用的pbuf
}

#endif /* LWIP_ICMP */
