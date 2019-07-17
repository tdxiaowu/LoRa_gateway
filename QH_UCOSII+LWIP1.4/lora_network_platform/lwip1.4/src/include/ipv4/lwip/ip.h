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
#ifndef __LWIP_IP_H__
#define __LWIP_IP_H__

#include "lwip/opt.h"

#include "lwip/def.h"
#include "lwip/pbuf.h"
#include "lwip/ip_addr.h"
#include "lwip/err.h"
#include "lwip/netif.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Currently, the function ip_output_if_opt() is only used with IGMP */
#define IP_OPTIONS_SEND   LWIP_IGMP
//定义IP数据报head首部长度
#define IP_HLEN 20
//定义IP数据报中协议类型字段的值
#define IP_PROTO_ICMP    1
#define IP_PROTO_IGMP    2
#define IP_PROTO_UDP     17
#define IP_PROTO_UDPLITE 136
#define IP_PROTO_TCP     6

/* This is passed as the destination address to ip_output_if (not
   to ip_output), meaning that an IP header already is constructed
   in the pbuf. This is used when TCP retransmits. */
#ifdef IP_HDRINCL
#undef IP_HDRINCL
#endif /* IP_HDRINCL */
#define IP_HDRINCL  NULL

#if LWIP_NETIF_HWADDRHINT
#define IP_PCB_ADDRHINT ;u8_t addr_hint
#else
#define IP_PCB_ADDRHINT
#endif /* LWIP_NETIF_HWADDRHINT */

/* This is the common part of all PCB types. It needs to be at the
   beginning of a PCB type definition. It is located here so that
   changes to this common part are made in one location instead of
   having to change all PCB structs. */
#define IP_PCB \
  /* ip addresses in network byte order */ \
  ip_addr_t local_ip; \
  ip_addr_t remote_ip; \
   /* Socket options */  \
  u8_t so_options;      \
   /* Type Of Service */ \
  u8_t tos;              \
  /* Time To Live */     \
  u8_t ttl               \
  /* link layer address resolution hint */ \
  IP_PCB_ADDRHINT

//定义IP控制块
struct ip_pcb {
/* Common members of all PCB types */
  IP_PCB;//宏IP_PCB相关的字段
};

/*
 * Option flags per-socket. These are the same like SO_XXX.
 */
/*#define SOF_DEBUG       0x01U     Unimplemented: turn on debugging info recording */
#define SOF_ACCEPTCONN    0x02U  /* socket has had listen() */
#define SOF_REUSEADDR     0x04U  /* allow local address reuse */
#define SOF_KEEPALIVE     0x08U  /* keep connections alive */
/*#define SOF_DONTROUTE   0x10U     Unimplemented: just use interface addresses */
#define SOF_BROADCAST     0x20U  /* permit to send and to receive broadcast messages (see IP_SOF_BROADCAST option) */
/*#define SOF_USELOOPBACK 0x40U     Unimplemented: bypass hardware when possible */
#define SOF_LINGER        0x80U  /* linger on close if data present */
/*#define SOF_OOBINLINE   0x0100U   Unimplemented: leave received OOB data in line */
/*#define SOF_REUSEPORT   0x0200U   Unimplemented: allow local address & port reuse */

/* These flags are inherited (e.g. from a listen-pcb to a connection-pcb): */
#define SOF_INHERITED   (SOF_REUSEADDR|SOF_KEEPALIVE|SOF_LINGER/*|SOF_DEBUG|SOF_DONTROUTE|SOF_OOBINLINE*/)


#ifdef PACK_STRUCT_USE_INCLUDES
#  include "arch/bpstruct.h"
#endif
//定义ip数据报首部结构体
PACK_STRUCT_BEGIN		//禁止编译器自对齐
struct ip_hdr {
  /* version / header length */
  PACK_STRUCT_FIELD(u8_t _v_hl);		//版本号＋首部长度
  /* type of service */
  PACK_STRUCT_FIELD(u8_t _tos);			//服务类型
  /* total length */
  PACK_STRUCT_FIELD(u16_t _len);		//总长度
  /* identification */
  PACK_STRUCT_FIELD(u16_t _id);			//标识字段
  /* fragment offset field */
  PACK_STRUCT_FIELD(u16_t _offset);		//３位标志位和13位片偏移字段
#define IP_RF 0x8000U        /* reserved fragment flag */
#define IP_DF 0x4000U        /* dont fragment flag */
#define IP_MF 0x2000U        /* more fragments flag 更多分片标志*/
#define IP_OFFMASK 0x1fffU   /* mask for fragmenting bits偏移量掩码 */
  /* time to live */
  PACK_STRUCT_FIELD(u8_t _ttl);			//TTL字段
  /* protocol*/
  PACK_STRUCT_FIELD(u8_t _proto);		//协议字段
  /* checksum */
  PACK_STRUCT_FIELD(u16_t _chksum);		//首部校验和字段
  /* source and destination IP addresses */
  PACK_STRUCT_FIELD(ip_addr_p_t src);	//源IP地址
  PACK_STRUCT_FIELD(ip_addr_p_t dest);	//目的IP地址
} PACK_STRUCT_STRUCT;
PACK_STRUCT_END
#ifdef PACK_STRUCT_USE_INCLUDES
#  include "arch/epstruct.h"
#endif

/*下面定义几个宏，用于对IP首部中各个字段值的读取，宏变量hdr为指向IP首部结果ip_hdr型变量的指针（用于接收时提取数据报内容，比较使用）*/
#define IPH_V(hdr)  ((hdr)->_v_hl >> 4)		//获取版本号
#define IPH_HL(hdr) ((hdr)->_v_hl & 0x0f)	//获取首部长度
#define IPH_TOS(hdr) ((hdr)->_tos)			//获取服务类型
#define IPH_LEN(hdr) ((hdr)->_len)			//获取数据报总长度（网络字节序）
#define IPH_ID(hdr) ((hdr)->_id)			//获取数据报标识字段（网络字节序）
#define IPH_OFFSET(hdr) ((hdr)->_offset)	//获取３位标志位和13位片偏移字段（网络字节序）
#define IPH_TTL(hdr) ((hdr)->_ttl)			//获取TTL字段
#define IPH_PROTO(hdr) ((hdr)->_proto)		//获取协议字段
#define IPH_CHKSUM(hdr) ((hdr)->_chksum)	//获取首部校验和字段（网络字节序）

/*下面定义几个宏，用于对IP首部中各个字段值的填写，宏变量hdr为指向IP首部结果ip_hdr型变量的指针（用于发送时组装数据报，把各部分写入数据报首部相应位置）*/
#define IPH_VHL_SET(hdr, v, hl) (hdr)->_v_hl = (((v) << 4) | (hl))		//填充版本号
#define IPH_TOS_SET(hdr, tos) (hdr)->_tos = (tos)						//填充服务类型
#define IPH_LEN_SET(hdr, len) (hdr)->_len = (len)						//填充数据报总长度（len应为网络字节序）
#define IPH_ID_SET(hdr, id) (hdr)->_id = (id)							//填充数据报标识字段（id网络字节序）
#define IPH_OFFSET_SET(hdr, off) (hdr)->_offset = (off)					//填充３位标志位和13位片偏移字段
#define IPH_TTL_SET(hdr, ttl) (hdr)->_ttl = (u8_t)(ttl)
#define IPH_PROTO_SET(hdr, proto) (hdr)->_proto = (u8_t)(proto)
#define IPH_CHKSUM_SET(hdr, chksum) (hdr)->_chksum = (chksum)			//填充首部校验和字段（chksum为网络字节序）

/** The interface that provided the packet for the current callback invocation. */
extern struct netif *current_netif;
/** Header of the input packet currently being processed. */
extern const struct ip_hdr *current_header;
/** Source IP address of current_header */
extern ip_addr_t current_iphdr_src;//外部引用
/** Destination IP address of current_header */
extern ip_addr_t current_iphdr_dest;//外部引用

#define ip_init() /* Compatibility define, not init needed. */
struct netif *ip_route(ip_addr_t *dest);
err_t ip_input(struct pbuf *p, struct netif *inp);
err_t ip_output(struct pbuf *p, ip_addr_t *src, ip_addr_t *dest,
       u8_t ttl, u8_t tos, u8_t proto);
err_t ip_output_if(struct pbuf *p, ip_addr_t *src, ip_addr_t *dest,
       u8_t ttl, u8_t tos, u8_t proto,
       struct netif *netif);
#if LWIP_NETIF_HWADDRHINT
err_t ip_output_hinted(struct pbuf *p, ip_addr_t *src, ip_addr_t *dest,
       u8_t ttl, u8_t tos, u8_t proto, u8_t *addr_hint);
#endif /* LWIP_NETIF_HWADDRHINT */
#if IP_OPTIONS_SEND
err_t ip_output_if_opt(struct pbuf *p, ip_addr_t *src, ip_addr_t *dest,
       u8_t ttl, u8_t tos, u8_t proto, struct netif *netif, void *ip_options,
       u16_t optlen);
#endif /* IP_OPTIONS_SEND */
/** Get the interface that received the current packet.
 * This function must only be called from a receive callback (udp_recv,
 * raw_recv, tcp_accept). It will return NULL otherwise. */
#define ip_current_netif()  (current_netif)
/** Get the IP header of the current packet.
 * This function must only be called from a receive callback (udp_recv,
 * raw_recv, tcp_accept). It will return NULL otherwise. */
#define ip_current_header() (current_header)
/** Source IP address of current_header */
#define ip_current_src_addr()  (&current_iphdr_src)
/** Destination IP address of current_header */
#define ip_current_dest_addr() (&current_iphdr_dest)

/** Gets an IP pcb option (SOF_* flags) */
#define ip_get_option(pcb, opt)   ((pcb)->so_options & (opt))
/** Sets an IP pcb option (SOF_* flags) */
#define ip_set_option(pcb, opt)   ((pcb)->so_options |= (opt))
/** Resets an IP pcb option (SOF_* flags) */
#define ip_reset_option(pcb, opt) ((pcb)->so_options &= ~(opt))

#if IP_DEBUG
void ip_debug_print(struct pbuf *p);
#else
#define ip_debug_print(p)
#endif /* IP_DEBUG */

#ifdef __cplusplus
}
#endif

#endif /* __LWIP_IP_H__ */


