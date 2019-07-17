/**
 * @file
 * Address Resolution Protocol module for IP over Ethernet
 *
 * Functionally, ARP is divided into two parts. The first maps an IP address
 * to a physical address when sending a packet, and the second part answers
 * requests from other machines for our physical address.
 *
 * This implementation complies with RFC 826 (Ethernet ARP). It supports
 * Gratuitious ARP from RFC3220 (IP Mobility Support for IPv4) section 4.6
 * if an interface calls etharp_gratuitous(our_netif) upon address change.
 */

/*
 * Copyright (c) 2001-2003 Swedish Institute of Computer Science.
 * Copyright (c) 2003-2004 Leon Woestenberg <leon.woestenberg@axon.tv>
 * Copyright (c) 2003-2004 Axon Digital Design B.V., The Netherlands.
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
 */
 
#include "lwip/opt.h"

#if LWIP_ARP || LWIP_ETHERNET

#include "lwip/ip_addr.h"
#include "lwip/def.h"
#include "lwip/ip.h"
#include "lwip/stats.h"
#include "lwip/snmp.h"
#include "lwip/dhcp.h"
#include "lwip/autoip.h"
#include "netif/etharp.h"

#if PPPOE_SUPPORT
#include "netif/ppp_oe.h"
#endif /* PPPOE_SUPPORT */

#include <string.h>
//定义特殊MAC地址的定义，以太网广播地址
const struct eth_addr ethbroadcast = {{0xff,0xff,0xff,0xff,0xff,0xff}};
//该值用于填充ARP请求包的接收方MAC字段，无实际意义
const struct eth_addr ethzero = {{0,0,0,0,0,0}};

/*宏定义，以太网多播地址　01-00-5e-xx-xx-xx*/
/** The 24-bit IANA multicast OUI is 01-00-5e: */
#define LL_MULTICAST_ADDR_0 0x01
#define LL_MULTICAST_ADDR_1 0x00
#define LL_MULTICAST_ADDR_2 0x5e

#if LWIP_ARP /* don't build if not configured for use in lwipopts.h */

/** the time an ARP entry stays valid after its last update,
 *  for ARP_TMR_INTERVAL = 5000, this is
 *  (240 * 5) seconds = 20 minutes.
 */
#define ARP_MAXAGE              240			//稳定状态表项的最大生存时间20min
/** Re-request a used ARP entry 1 minute before it would expire to prevent
 *  breaking a steadily used connection because the ARP entry timed out. */
#define ARP_AGE_REREQUEST_USED  (ARP_MAXAGE - 12)

/** the time an ARP entry stays pending after first request,
 *  for ARP_TMR_INTERVAL = 5000, this is
 *  (2 * 5) seconds = 10 seconds.
 * pending状态表项的最大生存时间　10s
 *  @internal Keep this number at least 2, otherwise it might
 *  run out instantly if the timeout occurs directly after a request.
 */
#define ARP_MAXPENDING 2

#define HWTYPE_ETHERNET 1

enum etharp_state {
  ETHARP_STATE_EMPTY = 0,			//空
  ETHARP_STATE_PENDING,				//挂起状态，表示该表项只记录了ip地址，还未记录对应MAC地址,计时10s,timeout delete entry
  ETHARP_STATE_STABLE,				//稳定状态，内核会周期性发ARP请求（时间是20min），维护其有效性
  ETHARP_STATE_STABLE_REREQUESTING	//稳定且发送一个ARP请求
#if ETHARP_SUPPORT_STATIC_ENTRIES
  ,ETHARP_STATE_STATIC
#endif /* ETHARP_SUPPORT_STATIC_ENTRIES */
};
//描述缓存表页的数据结构
struct etharp_entry {
#if ARP_QUEUEING
  /** Pointer to queue of pending outgoing packets on this ARP entry. */
  struct etharp_q_entry *q;
#else /* ARP_QUEUEING */
  /** Pointer to a single pending outgoing packet on this ARP entry. */
  struct pbuf *q;		//暂停发送的数据包指针
#endif /* ARP_QUEUEING */
  ip_addr_t ipaddr;		//目标ip地址
  struct netif *netif;	//对应网络接口信息
  struct eth_addr ethaddr;//mac地址
  u8_t state;			//描述该entry的状态
  u8_t ctime;			//描述该entry的时间信息
};

static struct etharp_entry arp_table[ARP_TABLE_SIZE];		//定义ARP缓存表10个

#if !LWIP_NETIF_HWADDRHINT
static u8_t etharp_cached_entry;
#endif /* !LWIP_NETIF_HWADDRHINT */

/** Try hard to create a new entry - we want the IP address to appear in
    the cache (even if this means removing an active entry or so). */
#define ETHARP_FLAG_TRY_HARD     1
#define ETHARP_FLAG_FIND_ONLY    2
#if ETHARP_SUPPORT_STATIC_ENTRIES
#define ETHARP_FLAG_STATIC_ENTRY 4
#endif /* ETHARP_SUPPORT_STATIC_ENTRIES */

#if LWIP_NETIF_HWADDRHINT
#define ETHARP_SET_HINT(netif, hint)  if (((netif) != NULL) && ((netif)->addr_hint != NULL))  \
                                      *((netif)->addr_hint) = (hint);
#else /* LWIP_NETIF_HWADDRHINT */
#define ETHARP_SET_HINT(netif, hint)  (etharp_cached_entry = (hint))
#endif /* LWIP_NETIF_HWADDRHINT */


/* Some checks, instead of etharp_init(): */
#if (LWIP_ARP && (ARP_TABLE_SIZE > 0x7f))
  #error "ARP_TABLE_SIZE must fit in an s8_t, you have to reduce it in your lwipopts.h"
#endif


#if ARP_QUEUEING
/**
 * Free a complete queue of etharp entries
 *
 * @param q a qeueue of etharp_q_entry's to free
 */
static void
free_etharp_q(struct etharp_q_entry *q)
{
  struct etharp_q_entry *r;
  LWIP_ASSERT("q != NULL", q != NULL);
  LWIP_ASSERT("q->p != NULL", q->p != NULL);
  while (q) {
    r = q;
    q = q->next;
    LWIP_ASSERT("r->p != NULL", (r->p != NULL));
    pbuf_free(r->p);
    memp_free(MEMP_ARP_QUEUE, r);
  }
}
#else /* ARP_QUEUEING */

/** Compatibility define: free the queued pbuf */
#define free_etharp_q(q) pbuf_free(q)

#endif /* ARP_QUEUEING */

/** Clean up ARP table entries */
static void
etharp_free_entry(int i)
{
  /* remove from SNMP ARP index tree */
  snmp_delete_arpidx_tree(arp_table[i].netif, &arp_table[i].ipaddr);
  /* and empty packet queue */
  if (arp_table[i].q != NULL) {
    /* remove all queued packets */
    LWIP_DEBUGF(ETHARP_DEBUG, ("etharp_free_entry: freeing entry %"U16_F", packet queue %p.\n", (u16_t)i, (void *)(arp_table[i].q)));
    free_etharp_q(arp_table[i].q);
    arp_table[i].q = NULL;
  }
  /* recycle entry for re-use */
  arp_table[i].state = ETHARP_STATE_EMPTY;
#ifdef LWIP_DEBUG
  /* for debugging, clean out the complete entry */
  arp_table[i].ctime = 0;
  arp_table[i].netif = NULL;
  ip_addr_set_zero(&arp_table[i].ipaddr);
  arp_table[i].ethaddr = ethzero;
#endif /* LWIP_DEBUG */
}

/**
 * Clears expired entries in the ARP table.
 * 该函数每５秒调用一次
 * This function should be called every ETHARP_TMR_INTERVAL milliseconds (5 seconds),
 * in order to expire entries in the ARP table.
 */
void
etharp_tmr(void)
{
  u8_t i;

  LWIP_DEBUGF(ETHARP_DEBUG, ("etharp_timer\n"));
  /* remove expired entries from the ARP table */
  for (i = 0; i < ARP_TABLE_SIZE; ++i) {
    u8_t state = arp_table[i].state;
    if (state != ETHARP_STATE_EMPTY
#if ETHARP_SUPPORT_STATIC_ENTRIES
      && (state != ETHARP_STATE_STATIC)
#endif /* ETHARP_SUPPORT_STATIC_ENTRIES */
      ) {
      arp_table[i].ctime++;
      if ((arp_table[i].ctime >= ARP_MAXAGE) ||
          ((arp_table[i].state == ETHARP_STATE_PENDING)  &&
           (arp_table[i].ctime >= ARP_MAXPENDING))) {
        /* pending or stable entry has become old! */
        LWIP_DEBUGF(ETHARP_DEBUG, ("etharp_timer: expired %s entry %"U16_F".\n",
             arp_table[i].state >= ETHARP_STATE_STABLE ? "stable" : "pending", (u16_t)i));
        /* clean up entries that have just been expired */
        etharp_free_entry(i);		//删除到期的表项
      }
      else if (arp_table[i].state == ETHARP_STATE_STABLE_REREQUESTING) {
        /* Reset state to stable, so that the next transmitted packet will
           re-send an ARP request. 设为stable状态，则在下次IP数据包发送前先发送了一个ARP请求*/
        arp_table[i].state = ETHARP_STATE_STABLE;
      }
#if ARP_QUEUEING
      /* still pending entry? (not expired) */
      if (arp_table[i].state == ETHARP_STATE_PENDING) {
        /* resend an ARP query here? */
      }
#endif /* ARP_QUEUEING */
    }
  }
}

/**为ipaddr寻找一个匹配的表项或者建立一个新的表项（由参数flags决定）
 * Search the ARP table for a matching or new entry.
 * 
 * If an IP address is given, return a pending or stable ARP entry that matches
 * the address. If no match is found, create a new entry with this address set,
 * but in state ETHARP_EMPTY. The caller must check and possibly change the
 * state of the returned entry.
 * 
 * If ipaddr is NULL, return a initialized new entry in state ETHARP_EMPTY.
 * 
 * In all cases, attempt to create new entries from an empty entry. If no
 * empty entries are available and ETHARP_FLAG_TRY_HARD flag is set, recycle
 * old entries. Heuristic choose the least important entry for recycling.
 *
 * @param ipaddr IP address to find in ARP cache, or to add if not found.
 * @param flags @see definition of ETHARP_FLAG_*
 * @param netif netif related to this address (used for NETIF_HWADDRHINT)
 *  
 * @return The ARP entry index that matched or is created, ERR_MEM if no
 * entry is found or could be recycled.
 */
static s8_t
etharp_find_entry(ip_addr_t *ipaddr, u8_t flags)
{
  s8_t old_pending = ARP_TABLE_SIZE, old_stable = ARP_TABLE_SIZE;
  s8_t empty = ARP_TABLE_SIZE;
  u8_t i = 0, age_pending = 0, age_stable = 0;
  /* oldest entry with packets on queue */
  s8_t old_queue = ARP_TABLE_SIZE;
  /* its age */
  u8_t age_queue = 0;

  /**
   * a) do a search through the cache, remember candidates
   * b) select candidate entry
   * c) create new entry
   */

  /* a) in a single search sweep, do all of this
   * 1) remember the first empty entry (if any)
   * 2) remember the oldest stable entry (if any)
   * 3) remember the oldest pending entry without queued packets (if any)
   * 4) remember the oldest pending entry with queued packets (if any)
   * 5) search for a matching IP entry, either pending or stable
   *    until 5 matches, or all entries are searched for.
   */
//顺序遍历整个ARP表
  for (i = 0; i < ARP_TABLE_SIZE; ++i) {
    u8_t state = arp_table[i].state;
    /* no empty entry found yet and now we do find one? */
    if ((empty == ARP_TABLE_SIZE) && (state == ETHARP_STATE_EMPTY)) {
      LWIP_DEBUGF(ETHARP_DEBUG, ("etharp_find_entry: found empty entry %"U16_F"\n", (u16_t)i));
      /* remember first empty entry */
      empty = i;
    } else if (state != ETHARP_STATE_EMPTY) {
      LWIP_ASSERT("state == ETHARP_STATE_PENDING || state >= ETHARP_STATE_STABLE",
        state == ETHARP_STATE_PENDING || state >= ETHARP_STATE_STABLE);
      /* if given, does IP address match IP address in ARP entry? */
      if (ipaddr && ip_addr_cmp(ipaddr, &arp_table[i].ipaddr)) {
        LWIP_DEBUGF(ETHARP_DEBUG | LWIP_DBG_TRACE, ("etharp_find_entry: found matching entry %"U16_F"\n", (u16_t)i));
        /* found exact IP address match, simply bail out */
        return i;
      }
      /* pending entry? */
      if (state == ETHARP_STATE_PENDING) {
        /* pending with queued packets? */
        if (arp_table[i].q != NULL) {
          if (arp_table[i].ctime >= age_queue) {
            old_queue = i;
            age_queue = arp_table[i].ctime;
          }
        } else
        /* pending without queued packets? */
        {
          if (arp_table[i].ctime >= age_pending) {
            old_pending = i;
            age_pending = arp_table[i].ctime;
          }
        }
      /* stable entry? */
      } else if (state >= ETHARP_STATE_STABLE) {
#if ETHARP_SUPPORT_STATIC_ENTRIES
        /* don't record old_stable for static entries since they never expire */
        if (state < ETHARP_STATE_STATIC)
#endif /* ETHARP_SUPPORT_STATIC_ENTRIES */
        {
          /* remember entry with oldest stable entry in oldest, its age in maxtime */
          if (arp_table[i].ctime >= age_stable) {
            old_stable = i;
            age_stable = arp_table[i].ctime;
          }
        }
      }
    }
  }
  /* { we have no match } => try to create a new entry */
   
  /* don't create new entry, only search? */
  if (((flags & ETHARP_FLAG_FIND_ONLY) != 0) ||
      /* or no empty entry found and not allowed to recycle? */
      ((empty == ARP_TABLE_SIZE) && ((flags & ETHARP_FLAG_TRY_HARD) == 0))) {
    LWIP_DEBUGF(ETHARP_DEBUG | LWIP_DBG_TRACE, ("etharp_find_entry: no empty entry found and not allowed to recycle\n"));
    return (s8_t)ERR_MEM;
  }
  
  /* b) choose the least destructive entry to recycle:
   * 1) empty entry
   * 2) oldest stable entry
   * 3) oldest pending entry without queued packets
   * 4) oldest pending entry with queued packets
   * 
   * { ETHARP_FLAG_TRY_HARD is set at this point }
   */ 

  /* 1) empty entry available? */
  if (empty < ARP_TABLE_SIZE) {
    i = empty;
    LWIP_DEBUGF(ETHARP_DEBUG | LWIP_DBG_TRACE, ("etharp_find_entry: selecting empty entry %"U16_F"\n", (u16_t)i));
  } else {
    /* 2) found recyclable stable entry? */
    if (old_stable < ARP_TABLE_SIZE) {
      /* recycle oldest stable*/
      i = old_stable;
      LWIP_DEBUGF(ETHARP_DEBUG | LWIP_DBG_TRACE, ("etharp_find_entry: selecting oldest stable entry %"U16_F"\n", (u16_t)i));
      /* no queued packets should exist on stable entries */
      LWIP_ASSERT("arp_table[i].q == NULL", arp_table[i].q == NULL);
    /* 3) found recyclable pending entry without queued packets? */
    } else if (old_pending < ARP_TABLE_SIZE) {
      /* recycle oldest pending */
      i = old_pending;
      LWIP_DEBUGF(ETHARP_DEBUG | LWIP_DBG_TRACE, ("etharp_find_entry: selecting oldest pending entry %"U16_F" (without queue)\n", (u16_t)i));
    /* 4) found recyclable pending entry with queued packets? */
    } else if (old_queue < ARP_TABLE_SIZE) {
      /* recycle oldest pending (queued packets are free in etharp_free_entry) */
      i = old_queue;
      LWIP_DEBUGF(ETHARP_DEBUG | LWIP_DBG_TRACE, ("etharp_find_entry: selecting oldest pending entry %"U16_F", freeing packet queue %p\n", (u16_t)i, (void *)(arp_table[i].q)));
      /* no empty or recyclable entries found */
    } else {
      LWIP_DEBUGF(ETHARP_DEBUG | LWIP_DBG_TRACE, ("etharp_find_entry: no empty or recyclable entries found\n"));
      return (s8_t)ERR_MEM;
    }

    /* { empty or recyclable entry found } */
    LWIP_ASSERT("i < ARP_TABLE_SIZE", i < ARP_TABLE_SIZE);
    etharp_free_entry(i);
  }

  LWIP_ASSERT("i < ARP_TABLE_SIZE", i < ARP_TABLE_SIZE);
  LWIP_ASSERT("arp_table[i].state == ETHARP_STATE_EMPTY",
    arp_table[i].state == ETHARP_STATE_EMPTY);

  /* IP address given? */
  if (ipaddr != NULL) {
    /* set IP address */
    ip_addr_copy(arp_table[i].ipaddr, *ipaddr);
  }
  arp_table[i].ctime = 0;
  return (err_t)i;
}

/**组装以太网帧头部，发送以太网帧
 * Send an IP packet on the network using netif->linkoutput
 * The ethernet header is filled in before sending.
 *
 * @params netif the lwIP network interface on which to send the packet
 * @params p the packet to send, p->payload pointing to the (uninitialized) ethernet header
 * @params src the source MAC address to be copied into the ethernet header
 * @params dst the destination MAC address to be copied into the ethernet header
 * @return ERR_OK if the packet was sent, any other err_t on failure
 */
static err_t
etharp_send_ip(struct netif *netif, struct pbuf *p, struct eth_addr *src, struct eth_addr *dst)
{
  struct eth_hdr *ethhdr = (struct eth_hdr *)p->payload;//定义一个以太网帧头指针并指向数据包中的以太网帧头部

  LWIP_ASSERT("netif->hwaddr_len must be the same as ETHARP_HWADDR_LEN for etharp!",
              (netif->hwaddr_len == ETHARP_HWADDR_LEN));
  ETHADDR32_COPY(&ethhdr->dest, dst);//填写目的MAC地址字段
  ETHADDR16_COPY(&ethhdr->src, src);//填写源MAC地址字段
  ethhdr->type = PP_HTONS(ETHTYPE_IP);//填写帧类型，此处为IP包
  LWIP_DEBUGF(ETHARP_DEBUG | LWIP_DBG_TRACE, ("etharp_send_ip: sending packet %p\n", (void *)p));
  /* send the packet */
  return netif->linkoutput(netif, p);//回调网卡数据包发送函数
}

/**更新ARP表
 * Update (or insert) a IP/MAC address pair in the ARP cache.
 *
 * If a pending entry is resolved, any queued packets will be sent
 * at this point.
 * 
 * @param netif netif related to this entry (used for NETIF_ADDRHINT)
 * @param ipaddr IP address of the inserted ARP entry.
 * @param ethaddr Ethernet address of the inserted ARP entry.
 * @param flags @see definition of ETHARP_FLAG_*
 *
 * @return
 * - ERR_OK Succesfully updated ARP cache.
 * - ERR_MEM If we could not add a new ARP entry when ETHARP_FLAG_TRY_HARD was set.
 * - ERR_ARG Non-unicast address given, those will not appear in ARP cache.
 *
 * @see pbuf_free()
 */
static err_t
etharp_update_arp_entry(struct netif *netif, ip_addr_t *ipaddr, struct eth_addr *ethaddr, u8_t flags)
{
  s8_t i;
  LWIP_ASSERT("netif->hwaddr_len == ETHARP_HWADDR_LEN", netif->hwaddr_len == ETHARP_HWADDR_LEN);
  LWIP_DEBUGF(ETHARP_DEBUG | LWIP_DBG_TRACE, ("etharp_update_arp_entry: %"U16_F".%"U16_F".%"U16_F".%"U16_F" - %02"X16_F":%02"X16_F":%02"X16_F":%02"X16_F":%02"X16_F":%02"X16_F"\n",
    ip4_addr1_16(ipaddr), ip4_addr2_16(ipaddr), ip4_addr3_16(ipaddr), ip4_addr4_16(ipaddr),
    ethaddr->addr[0], ethaddr->addr[1], ethaddr->addr[2],
    ethaddr->addr[3], ethaddr->addr[4], ethaddr->addr[5]));
  /* non-unicast address? */
  if (ip_addr_isany(ipaddr) ||
      ip_addr_isbroadcast(ipaddr, netif) ||
      ip_addr_ismulticast(ipaddr)) {
    LWIP_DEBUGF(ETHARP_DEBUG | LWIP_DBG_TRACE, ("etharp_update_arp_entry: will not add non-unicast IP address to ARP cache\n"));
    return ERR_ARG;
  }
  /* find or create ARP entry */
  i = etharp_find_entry(ipaddr, flags);
  /* bail out if no entry could be found */
  if (i < 0) {
    return (err_t)i;
  }

#if ETHARP_SUPPORT_STATIC_ENTRIES
  if (flags & ETHARP_FLAG_STATIC_ENTRY) {
    /* record static type */
    arp_table[i].state = ETHARP_STATE_STATIC;
  } else
#endif /* ETHARP_SUPPORT_STATIC_ENTRIES */
  {
    /* mark it stable */
    arp_table[i].state = ETHARP_STATE_STABLE;
  }

  /* record network interface */
  arp_table[i].netif = netif;
  /* insert in SNMP ARP index tree */
  snmp_insert_arpidx_tree(netif, &arp_table[i].ipaddr);

  LWIP_DEBUGF(ETHARP_DEBUG | LWIP_DBG_TRACE, ("etharp_update_arp_entry: updating stable entry %"S16_F"\n", (s16_t)i));
  /* update address */
  ETHADDR32_COPY(&arp_table[i].ethaddr, ethaddr);
  /* reset time stamp */
  arp_table[i].ctime = 0;
  /* this is where we will send out queued packets! */
  //如果该ARP表项上有未发送的缓存数据包，则把这些数据包发送出去
#if ARP_QUEUEING
  while (arp_table[i].q != NULL) {
    struct pbuf *p;
    /* remember remainder of queue */
    struct etharp_q_entry *q = arp_table[i].q;
    /* pop first item off the queue */
    arp_table[i].q = q->next;
    /* get the packet pointer */
    p = q->p;
    /* now queue entry can be freed */
    memp_free(MEMP_ARP_QUEUE, q);}
#else /* ARP_QUEUEING */
  if (arp_table[i].q != NULL) {//如果挂载缓存区不为空
    struct pbuf *p = arp_table[i].q;//定义一个数据包指针，并指向该缓存区
    arp_table[i].q = NULL;//把挂载缓冲区指针清空
#endif /* ARP_QUEUEING */
    //发送该ARP表项上未发送的缓存数据包
    /* send the queued IP packet */
    etharp_send_ip(netif, p, (struct eth_addr*)(netif->hwaddr), ethaddr);
    /* free the queued IP packet */
    pbuf_free(p);//释放掉原数据包缓存空间
  }
  return ERR_OK;
}

#if ETHARP_SUPPORT_STATIC_ENTRIES	//如果支持静态ARP缓存表
/** Add a new static entry to the ARP table. If an entry exists for the
 * specified IP address, this entry is overwritten.
 * If packets are queued for the specified IP address, they are sent out.
 *
 * @param ipaddr IP address for the new static entry
 * @param ethaddr ethernet address for the new static entry
 * @return @see return values of etharp_add_static_entry
 */
//向内核注册一个固定的（ip,mac）地址对
err_t
etharp_add_static_entry(ip_addr_t *ipaddr, struct eth_addr *ethaddr)
{
  struct netif *netif;
  LWIP_DEBUGF(ETHARP_DEBUG | LWIP_DBG_TRACE, ("etharp_add_static_entry: %"U16_F".%"U16_F".%"U16_F".%"U16_F" - %02"X16_F":%02"X16_F":%02"X16_F":%02"X16_F":%02"X16_F":%02"X16_F"\n",
    ip4_addr1_16(ipaddr), ip4_addr2_16(ipaddr), ip4_addr3_16(ipaddr), ip4_addr4_16(ipaddr),
    ethaddr->addr[0], ethaddr->addr[1], ethaddr->addr[2],
    ethaddr->addr[3], ethaddr->addr[4], ethaddr->addr[5]));

  netif = ip_route(ipaddr);
  if (netif == NULL) {
    return ERR_RTE;
  }

  return etharp_update_arp_entry(netif, ipaddr, ethaddr, ETHARP_FLAG_TRY_HARD | ETHARP_FLAG_STATIC_ENTRY);
}

/** Remove a static entry from the ARP table previously added with a call to
 * etharp_add_static_entry.
 *
 * @param ipaddr IP address of the static entry to remove
 * @return ERR_OK: entry removed
 *         ERR_MEM: entry wasn't found
 *         ERR_ARG: entry wasn't a static entry but a dynamic one
 */
err_t
etharp_remove_static_entry(ip_addr_t *ipaddr)
{
  s8_t i;
  LWIP_DEBUGF(ETHARP_DEBUG | LWIP_DBG_TRACE, ("etharp_remove_static_entry: %"U16_F".%"U16_F".%"U16_F".%"U16_F"\n",
    ip4_addr1_16(ipaddr), ip4_addr2_16(ipaddr), ip4_addr3_16(ipaddr), ip4_addr4_16(ipaddr)));

  /* find or create ARP entry */
  i = etharp_find_entry(ipaddr, ETHARP_FLAG_FIND_ONLY);
  /* bail out if no entry could be found */
  if (i < 0) {
    return (err_t)i;
  }

  if (arp_table[i].state != ETHARP_STATE_STATIC) {
    /* entry wasn't a static entry, cannot remove it */
    return ERR_ARG;
  }
  /* entry found, free it */
  etharp_free_entry(i);
  return ERR_OK;
}
#endif /* ETHARP_SUPPORT_STATIC_ENTRIES */

/**
 * Remove all ARP table entries of the specified netif.
 *
 * @param netif points to a network interface
 */
void etharp_cleanup_netif(struct netif *netif)
{
  u8_t i;

  for (i = 0; i < ARP_TABLE_SIZE; ++i) {
    u8_t state = arp_table[i].state;
    if ((state != ETHARP_STATE_EMPTY) && (arp_table[i].netif == netif)) {
      etharp_free_entry(i);
    }
  }
}

/**
 * Finds (stable) ethernet/IP address pair from ARP table
 * using interface and IP address index.
 * @note the addresses in the ARP table are in network order!
 *
 * @param netif points to interface index
 * @param ipaddr points to the (network order) IP address index
 * @param eth_ret points to return pointer
 * @param ip_ret points to return pointer
 * @return table index if found, -1 otherwise
 */
s8_t
etharp_find_addr(struct netif *netif, ip_addr_t *ipaddr,
         struct eth_addr **eth_ret, ip_addr_t **ip_ret)
{
  s8_t i;

  LWIP_ASSERT("eth_ret != NULL && ip_ret != NULL",
    eth_ret != NULL && ip_ret != NULL);

  LWIP_UNUSED_ARG(netif);

  i = etharp_find_entry(ipaddr, ETHARP_FLAG_FIND_ONLY);
  if((i >= 0) && (arp_table[i].state >= ETHARP_STATE_STABLE)) {
      *eth_ret = &arp_table[i].ethaddr;
      *ip_ret = &arp_table[i].ipaddr;
      return i;
  }
  return -1;
}

#if ETHARP_TRUST_IP_MAC
/**
 * Updates the ARP table using the given IP packet.
 *
 * Uses the incoming IP packet's source address to update the
 * ARP cache for the local network. The function does not alter
 * or free the packet. This function must be called before the
 * packet p is passed to the IP layer.
 *
 * @param netif The lwIP network interface on which the IP packet pbuf arrived.
 * @param p The IP packet that arrived on netif.
 *
 * @return NULL
 *
 * @see pbuf_free()
 */
static void
etharp_ip_input(struct netif *netif, struct pbuf *p)
{
  struct eth_hdr *ethhdr;
  struct ip_hdr *iphdr;
  ip_addr_t iphdr_src;
  LWIP_ERROR("netif != NULL", (netif != NULL), return;);

  /* Only insert an entry if the source IP address of the
     incoming IP packet comes from a host on the local network. */
  ethhdr = (struct eth_hdr *)p->payload;
  iphdr = (struct ip_hdr *)((u8_t*)ethhdr + SIZEOF_ETH_HDR);
#if ETHARP_SUPPORT_VLAN
  if (ethhdr->type == PP_HTONS(ETHTYPE_VLAN)) {
    iphdr = (struct ip_hdr *)((u8_t*)ethhdr + SIZEOF_ETH_HDR + SIZEOF_VLAN_HDR);
  }
#endif /* ETHARP_SUPPORT_VLAN */

  ip_addr_copy(iphdr_src, iphdr->src);

  /* source is not on the local network? */
  if (!ip_addr_netcmp(&iphdr_src, &(netif->ip_addr), &(netif->netmask))) {
    /* do nothing */
    return;
  }

  LWIP_DEBUGF(ETHARP_DEBUG | LWIP_DBG_TRACE, ("etharp_ip_input: updating ETHARP table.\n"));
  /* update the source IP address in the cache, if present */
  /* @todo We could use ETHARP_FLAG_TRY_HARD if we think we are going to talk
   * back soon (for example, if the destination IP address is ours. */
  etharp_update_arp_entry(netif, &iphdr_src, &(ethhdr->src), ETHARP_FLAG_FIND_ONLY);
}
#endif /* ETHARP_TRUST_IP_MAC */

/**函数功能：处理ARP数据包，更新ARP缓存表，对ARP请求进行应答
 * Responds to ARP requests to us. Upon ARP replies to us, add entry to cache  
 * send out queued IP packets. Updates cache with snooped address pairs.
 *
 * Should be called for incoming ARP packets. The pbuf in the argument
 * is freed by this function.
 *
 * @param netif The lwIP network interface on which the ARP packet pbuf arrived.本地网络接口
 * @param ethaddr Ethernet address of netif.本地网络接口的MAC地址
 * @param p The ARP packet that arrived on netif. Is freed by this function.接收到的ARP数据包，在此进行释放
 *
 * @return NULL
 *
 * @see pbuf_free()
 */
static void
etharp_arp_input(struct netif *netif, struct eth_addr *ethaddr, struct pbuf *p)
{
  struct etharp_hdr *hdr;//指向ARP数据包头部的指针
  struct eth_hdr *ethhdr;//指向以太网帧头部的指针
  /* these are aligned properly, whereas the ARP header fields might not be */
  ip_addr_t sipaddr, dipaddr;//暂存ARP包中源IP地址和目的IP地址
  u8_t for_us;//用于指示该ARP是否是发给本机的
#if LWIP_AUTOIP
  const u8_t * ethdst_hwaddr;
#endif /* LWIP_AUTOIP */

  LWIP_ERROR("netif != NULL", (netif != NULL), return;);

  //判断数据包中数据长度，不满足则释放
  /* drop short ARP packets: we have to check for p->len instead of p->tot_len here
     since a struct etharp_hdr is pointed to p->payload, so it musn't be chained! */
  if (p->len < SIZEOF_ETHARP_PACKET) {
    LWIP_DEBUGF(ETHARP_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_LEVEL_WARNING,
      ("etharp_arp_input: packet dropped, too short (%"S16_F"/%"S16_F")\n", p->tot_len,
      (s16_t)SIZEOF_ETHARP_PACKET));
    ETHARP_STATS_INC(etharp.lenerr);
    ETHARP_STATS_INC(etharp.drop);
    pbuf_free(p);
    return;
  }

  ethhdr = (struct eth_hdr *)p->payload;//ethhdr指向以太网帧首部
  hdr = (struct etharp_hdr *)((u8_t*)ethhdr + SIZEOF_ETH_HDR);//hdr指向ARP包头部
#if ETHARP_SUPPORT_VLAN
  if (ethhdr->type == PP_HTONS(ETHTYPE_VLAN)) {
    hdr = (struct etharp_hdr *)(((u8_t*)ethhdr) + SIZEOF_ETH_HDR + SIZEOF_VLAN_HDR);
  }
#endif /* ETHARP_SUPPORT_VLAN */
//判断ARP包各项是否符合协议规定
  /* RFC 826 "Packet Reception": */
  if ((hdr->hwtype != PP_HTONS(HWTYPE_ETHERNET)) ||
      (hdr->hwlen != ETHARP_HWADDR_LEN) ||
      (hdr->protolen != sizeof(ip_addr_t)) ||
      (hdr->proto != PP_HTONS(ETHTYPE_IP)))  {
    LWIP_DEBUGF(ETHARP_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_LEVEL_WARNING,
      ("etharp_arp_input: packet dropped, wrong hw type, hwlen, proto, protolen or ethernet type (%"U16_F"/%"U16_F"/%"U16_F"/%"U16_F")\n",
      hdr->hwtype, hdr->hwlen, hdr->proto, hdr->protolen));
    ETHARP_STATS_INC(etharp.proterr);
    ETHARP_STATS_INC(etharp.drop);
    pbuf_free(p);
    return;
  }
  ETHARP_STATS_INC(etharp.recv);

#if LWIP_AUTOIP
  /* We have to check if a host already has configured our random
   * created link local address and continously check if there is
   * a host with this IP-address so we can detect collisions */
  autoip_arp_reply(netif, hdr);
#endif /* LWIP_AUTOIP */

  //由于ARP数据包中的IP地址字段并不是字对齐的，不能直接使用hdr中是定义了两个16字节的数组，ip_addr_t是定义的４字节变量
  /* Copy struct ip_addr2 to aligned ip_addr, to support compilers without
   * structure packing (not using structure copy which breaks strict-aliasing rules). */
  IPADDR2_COPY(&sipaddr, &hdr->sipaddr);//拷贝发送方IP地址到sipaddr
  IPADDR2_COPY(&dipaddr, &hdr->dipaddr);//拷贝接收方IP地址到dipaddr

  /* this interface is not configured? */
  if (ip_addr_isany(&netif->ip_addr)) {//如果网卡IP地址未配置
    for_us = 0;
  } else {
    /* ARP packet directed to us? */
    for_us = (u8_t)ip_addr_cmp(&dipaddr, &(netif->ip_addr));//若相等，for_us被置１
  }
//更新ARP缓存表
  /* ARP message directed to us?
      -> add IP address in ARP cache; assume requester wants to talk to us,
         can result in directly sending the queued packets for this host.
     ARP message not directed to us?
      ->  update the source IP address in the cache, if present */
  etharp_update_arp_entry(netif, &sipaddr, &(hdr->shwaddr),
                   for_us ? ETHARP_FLAG_TRY_HARD : ETHARP_FLAG_FIND_ONLY);

  /* now act on the message itself */
  switch (hdr->opcode) {//判断ARP数据包类型
  /* ARP request? */
  case PP_HTONS(ARP_REQUEST)://如果是ARP请求
    /* ARP request. If it asked for our address, we send out a
     * reply. In any case, we time-stamp any existing ARP entry,
     * and possiby send out an IP packet that was queued on it. */

    LWIP_DEBUGF (ETHARP_DEBUG | LWIP_DBG_TRACE, ("etharp_arp_input: incoming ARP request\n"));
    /* ARP request for our address? */
    if (for_us) {

      LWIP_DEBUGF(ETHARP_DEBUG | LWIP_DBG_TRACE, ("etharp_arp_input: replying to ARP request for our IP address\n"));
      /* Re-use pbuf to send ARP reply.
         Since we are re-using an existing pbuf, we can't call etharp_raw since
         that would allocate a new pbuf. */
      hdr->opcode = htons(ARP_REPLY);//将OP字段改为ARP响应类型，做成响应数据包

      IPADDR2_COPY(&hdr->dipaddr, &hdr->sipaddr);//设置接收端ip地址
      IPADDR2_COPY(&hdr->sipaddr, &netif->ip_addr);//设置发送端ip地址为本机ip地址

      LWIP_ASSERT("netif->hwaddr_len must be the same as ETHARP_HWADDR_LEN for etharp!",
                  (netif->hwaddr_len == ETHARP_HWADDR_LEN));
#if LWIP_AUTOIP
      /* If we are using Link-Local, all ARP packets that contain a Link-Local
       * 'sender IP address' MUST be sent using link-layer broadcast instead of
       * link-layer unicast. (See RFC3927 Section 2.5, last paragraph) */
      ethdst_hwaddr = ip_addr_islinklocal(&netif->ip_addr) ? (u8_t*)(ethbroadcast.addr) : hdr->shwaddr.addr;
#endif /* LWIP_AUTOIP */

      ETHADDR16_COPY(&hdr->dhwaddr, &hdr->shwaddr);//设置ARP首部中接收端MAC
#if LWIP_AUTOIP
      ETHADDR16_COPY(&ethhdr->dest, ethdst_hwaddr);
#else  /* LWIP_AUTOIP */
      ETHADDR16_COPY(&ethhdr->dest, &hdr->shwaddr);//设置以太网帧头部目标MAC
#endif /* LWIP_AUTOIP */
      ETHADDR16_COPY(&hdr->shwaddr, ethaddr);//设置ARP首部中发送端MAC
      ETHADDR16_COPY(&ethhdr->src, ethaddr);//设置以太网帧头部源MAC

      /* hwtype, hwaddr_len, proto, protolen and the type in the ethernet header
         are already correct, we tested that before */

      /* return ARP reply */
      netif->linkoutput(netif, p);//直接发送ARP应答包
    /* we are not configured? */
    } else if (ip_addr_isany(&netif->ip_addr)) {//如果网卡IP地址未配置
      /* { for_us == 0 and netif->ip_addr.addr == 0 } */
      LWIP_DEBUGF(ETHARP_DEBUG | LWIP_DBG_TRACE, ("etharp_arp_input: we are unconfigured, ARP request ignored.\n"));
    /* request was not directed to us */
    } else {
      /* { for_us == 0 and netif->ip_addr.addr != 0 } */
      LWIP_DEBUGF(ETHARP_DEBUG | LWIP_DBG_TRACE, ("etharp_arp_input: ARP request was not for us.\n"));
    }
    break;
  case PP_HTONS(ARP_REPLY)://如果是ARP应答，已经在最开始更新了ARP表，不做任何操作
    /* ARP reply. We already updated the ARP cache earlier. */
    LWIP_DEBUGF(ETHARP_DEBUG | LWIP_DBG_TRACE, ("etharp_arp_input: incoming ARP reply\n"));
#if (LWIP_DHCP && DHCP_DOES_ARP_CHECK)
    /* DHCP wants to know about ARP replies from any host with an
     * IP address also offered to us by the DHCP server. We do not
     * want to take a duplicate IP address on a single network.
     * @todo How should we handle redundant (fail-over) interfaces? */
    dhcp_arp_reply(netif, &sipaddr);
#endif /* (LWIP_DHCP && DHCP_DOES_ARP_CHECK) */
    break;
  default:
    LWIP_DEBUGF(ETHARP_DEBUG | LWIP_DBG_TRACE, ("etharp_arp_input: ARP unknown opcode type %"S16_F"\n", htons(hdr->opcode)));
    ETHARP_STATS_INC(etharp.err);
    break;
  }
  /* free ARP packet */
  pbuf_free(p);//释放数据包
}

/** Just a small helper function that sends a pbuf to an ethernet address
 * in the arp_table specified by the index 'arp_idx'.
 */
static err_t
etharp_output_to_arp_index(struct netif *netif, struct pbuf *q, u8_t arp_idx)
{
  LWIP_ASSERT("arp_table[arp_idx].state >= ETHARP_STATE_STABLE",
              arp_table[arp_idx].state >= ETHARP_STATE_STABLE);
  /* if arp table entry is about to expire: re-request it,
     but only if its state is ETHARP_STATE_STABLE to prevent flooding the
     network with ARP requests if this address is used frequently. */
  if ((arp_table[arp_idx].state == ETHARP_STATE_STABLE) && 
      (arp_table[arp_idx].ctime >= ARP_AGE_REREQUEST_USED)) {
    if (etharp_request(netif, &arp_table[arp_idx].ipaddr) == ERR_OK) {
      arp_table[arp_idx].state = ETHARP_STATE_STABLE_REREQUESTING;
    }
  }
  
  return etharp_send_ip(netif, q, (struct eth_addr*)(netif->hwaddr),
    &arp_table[arp_idx].ethaddr);
}

/**函数功能：发送一个ip数据包pbuf到目的地址ipaddr处，此函数被ip层调用
 * Resolve and fill-in Ethernet address header for outgoing IP packet.
 *
 * For IP multicast and broadcast, corresponding Ethernet addresses
 * are selected and the packet is transmitted on the link.
 *
 * For unicast addresses, the packet is submitted to etharp_query(). In
 * case the IP address is outside the local network, the IP address of
 * the gateway is used.
 *
 * @param netif The lwIP network interface which the IP packet will be sent on.
 * @param q The pbuf(s) containing the IP packet to be sent.
 * @param ipaddr The IP address of the packet destination.
 *
 * @return
 * - ERR_RTE No route to destination (no gateway to external networks),
 * or the return type of either etharp_query() or etharp_send_ip().
 */
err_t
etharp_output(struct netif *netif, struct pbuf *q, ip_addr_t *ipaddr)
{
  struct eth_addr *dest;
  struct eth_addr mcastaddr;
  ip_addr_t *dst_addr = ipaddr;		//初始化目的ip地址

  LWIP_ASSERT("netif != NULL", netif != NULL);
  LWIP_ASSERT("q != NULL", q != NULL);
  LWIP_ASSERT("ipaddr != NULL", ipaddr != NULL);

  /*调整pbuf的payload指针，使其指向以太网帧头部，失败则返回*/
  /* make room for Ethernet header - should not fail */
  if (pbuf_header(q, sizeof(struct eth_hdr)) != 0) {
    /* bail out */
    LWIP_DEBUGF(ETHARP_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_LEVEL_SERIOUS,
      ("etharp_output: could not allocate room for header.\n"));
    LINK_STATS_INC(link.lenerr);
    return ERR_BUF;//-2
  }

  /* Determine on destination hardware address. Broadcasts and multicasts
   * are special, other IP addresses are looked up in the ARP table. */

  /* broadcast destination IP address? */
  if (ip_addr_isbroadcast(ipaddr, netif)) {		//如果是广播ip地址
    /* broadcast on Ethernet also */
    dest = (struct eth_addr *)&ethbroadcast;	//dest指向广播MAC地址
  /* multicast destination IP address? */
  } else if (ip_addr_ismulticast(ipaddr)) {		//如果是多播地址，构造多播地址
    /* Hash IP multicast address to MAC address.*/
    mcastaddr.addr[0] = LL_MULTICAST_ADDR_0;
    mcastaddr.addr[1] = LL_MULTICAST_ADDR_1;
    mcastaddr.addr[2] = LL_MULTICAST_ADDR_2;
    mcastaddr.addr[3] = ip4_addr2(ipaddr) & 0x7f;
    mcastaddr.addr[4] = ip4_addr3(ipaddr);
    mcastaddr.addr[5] = ip4_addr4(ipaddr);
    /* destination Ethernet address is multicast */
    dest = &mcastaddr;		//dest指向多播MAC地址
  /* unicast destination IP address? */
  } else {		//如果是单播ip地址
    s8_t i;
    /* outside local network? if so, this can neither be a global broadcast nor
       a subnet broadcast. */
    /*判断目的ip地址是否为本地的子网，若不在，则修改ipaddr*/
    if (!ip_addr_netcmp(ipaddr, &(netif->ip_addr), &(netif->netmask)) &&
        !ip_addr_islinklocal(ipaddr)) {
#if LWIP_AUTOIP
      struct ip_hdr *iphdr = (struct ip_hdr*)((u8_t*)q->payload +
        sizeof(struct eth_hdr));
      /* According to RFC 3297, chapter 2.6.2 (Forwarding Rules), a packet with
         a link-local source address must always be "directly to its destination
         on the same physical link. The host MUST NOT send the packet to any
         router for forwarding". */
      if (!ip_addr_islinklocal(&iphdr->src))
#endif /* LWIP_AUTOIP */
      {
    	/*需要将数据包发送到网关处，由网关转发*/
        /* interface has default gateway? */
        if (!ip_addr_isany(&netif->gw)) {
          /* send to hardware address of default gateway IP address */
          dst_addr = &(netif->gw);		//更改目的的ip地址，将数据包发往到网关处
        /* no default gateway available */
        } else {
          /* no route to destination error (default gateway m－４issing) */
          return ERR_RTE;		//如果网关未配置，返回错误
        }
      }
    }
#if LWIP_NETIF_HWADDRHINT
    if (netif->addr_hint != NULL) {
      /* per-pcb cached entry was given */
      u8_t etharp_cached_entry = *(netif->addr_hint);
      if (etharp_cached_entry < ARP_TABLE_SIZE) {
#endif /* LWIP_NETIF_HWADDRHINT */
        if ((arp_table[etharp_cached_entry].state >= ETHARP_STATE_STABLE) &&
            (ip_addr_cmp(dst_addr, &arp_table[etharp_cached_entry].ipaddr))) {
          /* the per-pcb-cached entry is stable and the right one! */
          ETHARP_STATS_INC(etharp.cachehit);
          return etharp_output_to_arp_index(netif, q, etharp_cached_entry);
        }
#if LWIP_NETIF_HWADDRHINT
      }
    }
#endif /* LWIP_NETIF_HWADDRHINT */

    /* find stable entry: do this here since this is a critical path for
       throughput and etharp_find_entry() is kind of slow */
    for (i = 0; i < ARP_TABLE_SIZE; i++) {
      if ((arp_table[i].state >= ETHARP_STATE_STABLE) &&
          (ip_addr_cmp(dst_addr, &arp_table[i].ipaddr))) {
        /* found an existing, stable entry */
        ETHARP_SET_HINT(netif, i);
        return etharp_output_to_arp_index(netif, q, i);
      }
    }
    /*对于单播包，调用etharp_query查询其MAC地址并发送数据包*/
    /* no stable entry found, use the (slower) query function:
       queue on destination Ethernet address belonging to ipaddr */
    return etharp_query(netif, dst_addr, q);
  }

  /*对于多播和广播包，由于得到了它们的目的MAC地址，所以可以直接发送*/
  /* continuation for multicast/broadcast destinations */
  /* obtain source Ethernet address of the given interface */
  /* send packet directly on the link */
  return etharp_send_ip(netif, q, (struct eth_addr*)(netif->hwaddr), dest);
}

/**查找单播IP地址对应的MAC地址，并发送数据包
 * Send an ARP request for the given IP address and/or queue a packet.
 *
 * If the IP address was not yet in the cache, a pending ARP cache entry
 * is added and an ARP request is sent for the given address. The packet
 * is queued on this entry.
 *
 * If the IP address was already pending in the cache, a new ARP request
 * is sent for the given address. The packet is queued on this entry.
 *
 * If the IP address was already stable in the cache, and a packet is
 * given, it is directly sent and no ARP request is sent out. 
 * 
 * If the IP address was already stable in the cache, and no packet is
 * given, an ARP request is sent out.
 * 
 * @param netif The lwIP network interface on which ipaddr
 * must be queried for.
 * @param ipaddr The IP address to be resolved.
 * @param q If non-NULL, a pbuf that must be delivered to the IP address.
 * q is not freed by this function.
 *
 * @note q must only be ONE packet, not a packet queue!
 *
 * @return
 * - ERR_BUF Could not make room for Ethernet header.
 * - ERR_MEM Hardware address unknown, and no more ARP entries available
 *   to query for address or queue the packet.
 * - ERR_MEM Could not queue packet due to memory shortage.
 * - ERR_RTE No route to destination (no gateway to external networks).
 * - ERR_ARG Non-unicast address given, those will not appear in ARP cache.
 *
 */
err_t
etharp_query(struct netif *netif, ip_addr_t *ipaddr, struct pbuf *q)
{
  struct eth_addr * srcaddr = (struct eth_addr *)netif->hwaddr;
  err_t result = ERR_MEM;
  s8_t i; /* ARP entry index */

  //查找单播地址
  /* non-unicast address? */
  if (ip_addr_isbroadcast(ipaddr, netif) ||
      ip_addr_ismulticast(ipaddr) ||
      ip_addr_isany(ipaddr)) {
    LWIP_DEBUGF(ETHARP_DEBUG | LWIP_DBG_TRACE, ("etharp_query: will not add non-unicast IP address to ARP cache\n"));
    return ERR_ARG;
  }
  //调用find_entry查找或创建一个ARP表项
  /* find entry in ARP cache, ask to create entry if queueing packet */
  i = etharp_find_entry(ipaddr, ETHARP_FLAG_TRY_HARD);
  //若查找失败
  /* could not find or create entry? */
  if (i < 0) {
    LWIP_DEBUGF(ETHARP_DEBUG | LWIP_DBG_TRACE, ("etharp_query: could not create ARP entry\n"));
    if (q) {
      LWIP_DEBUGF(ETHARP_DEBUG | LWIP_DBG_TRACE, ("etharp_query: packet dropped\n"));
      ETHARP_STATS_INC(etharp.memerr);
    }
    return (err_t)i;
  }
  //如果表项的状态为empty，说明表项是刚创建的，且其中已经记录了IP地址
  /* mark a fresh entry as pending (we just sent a request) */
  if (arp_table[i].state == ETHARP_STATE_EMPTY) {
    arp_table[i].state = ETHARP_STATE_PENDING;
  }

  /* { i is either a STABLE or (new or existing) PENDING entry } */
  LWIP_ASSERT("arp_table[i].state == PENDING or STABLE",
  ((arp_table[i].state == ETHARP_STATE_PENDING) ||
   (arp_table[i].state >= ETHARP_STATE_STABLE)));
  //表项为pending态，或数据包为空，发送ARP请求包
  /* do we have a pending entry? or an implicit query request? */
  if ((arp_table[i].state == ETHARP_STATE_PENDING) || (q == NULL)) {
    /* try to resolve it; send out ARP request */
    result = etharp_request(netif, ipaddr);
    if (result != ERR_OK) {
      /* ARP request couldn't be sent */
      /* We don't re-send arp request in etharp_tmr, but we still queue packets,
         since this failure could be temporary, and the next packet calling
         etharp_query again could lead to sending the queued packets. */
    }
    if (q == NULL) {
      return result;
    }
  }
  //数据包不为空，则进行数据包的发送或者将数据包挂接在缓冲队列上
  /* packet given? */
  LWIP_ASSERT("q != NULL", q != NULL);
  /* stable entry? */
  if (arp_table[i].state >= ETHARP_STATE_STABLE) {//ARP表稳定，则直接发送数据包
    /* we have a valid IP->Ethernet address mapping */
    ETHARP_SET_HINT(netif, i);
    /* send the packet */
    result = etharp_send_ip(netif, q, srcaddr, &(arp_table[i].ethaddr));
  /* pending entry? (either just created or already pending */
  } else if (arp_table[i].state == ETHARP_STATE_PENDING) {//否则，挂接数据包
    /* entry is still pending, queue the given packet 'q' */
    struct pbuf *p;
    int copy_needed = 0;//是否需要重新拷贝整个数据包，数据包全由PBUF_ROM类型的pbuf组成时，才不需要拷贝
    /* IF q includes a PBUF_REF, PBUF_POOL or PBUF_RAM, we have no choice but
     * to copy the whole queue into a new PBUF_RAM (see bug #11400) 
     * PBUF_ROMs can be left as they are, since ROM must not get changed. */
    p = q;
    //判断是否需要拷贝
    while (p) {
      LWIP_ASSERT("no packet queues allowed!", (p->len != p->tot_len) || (p->next == 0));
      if(p->type != PBUF_ROM) {
        copy_needed = 1;
        break;
      }
      p = p->next;
    }
    //如果需要拷贝，则申请内存堆空间
    if(copy_needed) {
      /* copy the whole packet into new pbufs */
      p = pbuf_alloc(PBUF_RAW, p->tot_len, PBUF_RAM);//申请一个pbuf空间
      if(p != NULL) {
        if (pbuf_copy(p, q) != ERR_OK) {//拷贝失败，则释放申请的空间
          pbuf_free(p);
          p = NULL;
        }
      }
    } else {//如果不需要拷贝
      /* referencing the old pbuf is enough */
      p = q;//设置ｐ
      pbuf_ref(p);//增加pbuf的ref值
    }//到这里，p指向了我们需要挂接的数据包，下面执行挂接操作
    /* packet could be taken over? */

    if (p != NULL) { //如果挂载数据包不为空，则挂载
      /* queue packet ... */
#if ARP_QUEUEING
      struct etharp_q_entry *new_entry;
      /* allocate a new arp queue entry */
      new_entry = (struct etharp_q_entry *)memp_malloc(MEMP_ARP_QUEUE);
      if (new_entry != NULL) {
        new_entry->next = 0;
        new_entry->p = p;
        if(arp_table[i].q != NULL) {
          /* queue was already existent, append the new entry to the end */
          struct etharp_q_entry *r;
          r = arp_table[i].q;
          while (r->next != NULL) {
            r = r->next;
          }
          r->next = new_entry;
        } else {
          /* queue did not exist, first item in queue */
          arp_table[i].q = new_entry;
        }
        LWIP_DEBUGF(ETHARP_DEBUG | LWIP_DBG_TRACE, ("etharp_query: queued packet %p on ARP entry %"S16_F"\n", (void *)q, (s16_t)i));
        result = ERR_OK;
      } else {
        /* the pool MEMP_ARP_QUEUE is empty */
        pbuf_free(p);
        LWIP_DEBUGF(ETHARP_DEBUG | LWIP_DBG_TRACE, ("etharp_query: could not queue a copy of PBUF_REF packet %p (out of memory)\n", (void *)q));
        result = ERR_MEM;
      }
#else /* ARP_QUEUEING */
      /* always queue one packet per ARP request only, freeing a previously queued packet */
      //始终只为每个ARP请求排队一个数据包，释放先前排队的数据包
      if (arp_table[i].q != NULL) {//如果ARP表项挂载缓存区不为空，释放掉挂载项缓存区
        LWIP_DEBUGF(ETHARP_DEBUG | LWIP_DBG_TRACE, ("etharp_query: dropped previously queued packet %p for ARP entry %"S16_F"\n", (void *)q, (s16_t)i));
        pbuf_free(arp_table[i].q);
      }
      arp_table[i].q = p;//将ARP表项挂载区指针指向数据缓存区，该挂载项会在arp表更新函数中被发送
      result = ERR_OK;
      LWIP_DEBUGF(ETHARP_DEBUG | LWIP_DBG_TRACE, ("etharp_query: queued packet %p on ARP entry %"S16_F"\n", (void *)q, (s16_t)i));
#endif /* ARP_QUEUEING */
    } else {//如果挂载数据包为空，不操作
      ETHARP_STATS_INC(etharp.memerr);
      LWIP_DEBUGF(ETHARP_DEBUG | LWIP_DBG_TRACE, ("etharp_query: could not queue a copy of PBUF_REF packet %p (out of memory)\n", (void *)q));
      result = ERR_MEM;
    }
  }
  return result;
}

/**
 * Send a raw ARP packet (opcode and all addresses can be modified)
 *函数功能，根据各个参数字段组织一个ARP数据包并发送
 * @param netif the lwip network interface on which to send the ARP packet
 * @param ethsrc_addr the source MAC address for the ethernet header
 * @param ethdst_addr the destination MAC address for the ethernet header
 * @param hwsrc_addr the source MAC address for the ARP protocol header
 * @param ipsrc_addr the source IP address for the ARP protocol header
 * @param hwdst_addr the destination MAC address for the ARP protocol header
 * @param ipdst_addr the destination IP address for the ARP protocol header
 * @param opcode the type of the ARP packet
 * @return ERR_OK if the ARP packet has been sent
 *         ERR_MEM if the ARP packet couldn't be allocated
 *         any other err_t on failure
 * 注：ARP数据包中其他字段使用预定义值，例如硬件地址长度６，协议地址长度４
 */
#if !LWIP_AUTOIP
static
#endif /* LWIP_AUTOIP */
err_t
etharp_raw(struct netif *netif, const struct eth_addr *ethsrc_addr,
           const struct eth_addr *ethdst_addr,
           const struct eth_addr *hwsrc_addr, const ip_addr_t *ipsrc_addr,
           const struct eth_addr *hwdst_addr, const ip_addr_t *ipdst_addr,
           const u16_t opcode)
{
  struct pbuf *p;			//数据包指针
  err_t result = ERR_OK;	//返回结果
  struct eth_hdr *ethhdr;	//以太网数据帧首部结构体指针
  struct etharp_hdr *hdr;	//ARP数据包结构体指针
#if LWIP_AUTOIP
  const u8_t * ethdst_hwaddr;
#endif /* LWIP_AUTOIP */

  LWIP_ASSERT("netif != NULL", netif != NULL);

  /* allocate a pbuf for the outgoing ARP request packet */
  /*在内存堆中为ARP包分配空间，大小为ARP包长度*/
  p = pbuf_alloc(PBUF_RAW, SIZEOF_ETHARP_PACKET, PBUF_RAM);
  /* could allocate a pbuf for an ARP request? */
  if (p == NULL) {
    LWIP_DEBUGF(ETHARP_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_LEVEL_SERIOUS,
      ("etharp_raw: could not allocate pbuf for ARP request.\n"));
    ETHARP_STATS_INC(etharp.memerr);
    return ERR_MEM;
  }
  LWIP_ASSERT("check that first pbuf can hold struct etharp_hdr",
              (p->len >= SIZEOF_ETHARP_PACKET));

  ethhdr = (struct eth_hdr *)p->payload;	//ethhdr指向以太网帧首部区域
  hdr = (struct etharp_hdr *)((u8_t*)ethhdr + SIZEOF_ETH_HDR);	//hdr指向ARP首部
  LWIP_DEBUGF(ETHARP_DEBUG | LWIP_DBG_TRACE, ("etharp_raw: sending raw ARP packet.\n"));
  hdr->opcode = htons(opcode);		//填写ARP包的OP字段，注意大小端的转换

  LWIP_ASSERT("netif->hwaddr_len must be the same as ETHARP_HWADDR_LEN for etharp!",
              (netif->hwaddr_len == ETHARP_HWADDR_LEN));
#if LWIP_AUTOIP
  /* If we are using Link-Local, all ARP packets that contain a Link-Local
   * 'sender IP address' MUST be sent using link-layer broadcast instead of
   * link-layer unicast. (See RFC3927 Section 2.5, last paragraph) */
  ethdst_hwaddr = ip_addr_islinklocal(ipsrc_addr) ? (u8_t*)(ethbroadcast.addr) : ethdst_addr->addr;
#endif /* LWIP_AUTOIP */
  /* Write the ARP MAC-Addresses */
  ETHADDR16_COPY(&hdr->shwaddr, hwsrc_addr);//ARP头部的发送方MAC地址
  ETHADDR16_COPY(&hdr->dhwaddr, hwdst_addr);//ARP头部的接收方MAC地址
  /* Write the Ethernet MAC-Addresses */
#if LWIP_AUTOIP
  ETHADDR16_COPY(&ethhdr->dest, ethdst_hwaddr);
#else  /* LWIP_AUTOIP */
  ETHADDR16_COPY(&ethhdr->dest, ethdst_addr);//以太网帧首部中目的MAC地址
#endif /* LWIP_AUTOIP */
  ETHADDR16_COPY(&ethhdr->src, ethsrc_addr);//以太网帧首部中源MAC地址
  /* Copy struct ip_addr2 to aligned ip_addr, to support compilers without
   * structure packing. */ 
  IPADDR2_COPY(&hdr->sipaddr, ipsrc_addr);//填写ARP头部的发送方IP地址
  IPADDR2_COPY(&hdr->dipaddr, ipdst_addr);//填写ARP头部的接收方IP地址
  //下面填充一些固定字段的值
  hdr->hwtype = PP_HTONS(HWTYPE_ETHERNET);//ARP头部的硬件类型１，即以太网
  hdr->proto = PP_HTONS(ETHTYPE_IP);		//ARP头部的协议类型为0x0800
  /* set hwlen and protolen */
  hdr->hwlen = ETHARP_HWADDR_LEN;
  hdr->protolen = sizeof(ip_addr_t);

  ethhdr->type = PP_HTONS(ETHTYPE_ARP);//以太网帧首部中的帧类型字段，ARP包
  /* send ARP query */
  result = netif->linkoutput(netif, p);//调用底层数据包发送函数
  ETHARP_STATS_INC(etharp.xmit);
  /* free ARP query packet */
  pbuf_free(p);//释放数据包
  p = NULL;//p指向空，防止成为野指针
  /* could not allocate pbuf for ARP request */

  return result;
}

/**发送一个arp请求，请求一个ip地址的MAC地址
 * Send an ARP request packet asking for ipaddr.
 *
 * @param netif the lwip network interface on which to send the request
 * @param ipaddr the IP address for which to ask
 * @return ERR_OK if the request has been sent
 *         ERR_MEM if the ARP packet couldn't be allocated
 *         any other err_t on failure
 */
err_t
etharp_request(struct netif *netif, ip_addr_t *ipaddr)
{
  LWIP_DEBUGF(ETHARP_DEBUG | LWIP_DBG_TRACE, ("etharp_request: sending ARP request.\n"));
  return etharp_raw(netif, (struct eth_addr *)netif->hwaddr, &ethbroadcast,
                    (struct eth_addr *)netif->hwaddr, &netif->ip_addr, &ethzero,
                    ipaddr, ARP_REQUEST);
}
#endif /* LWIP_ARP */

/**以太网帧信息输入函数，处理接收到的以太网帧
 * Process received ethernet frames. Using this function instead of directly
 * calling ip_input and passing ARP frames through etharp in ethernetif_input,
 * the ARP cache is protected from concurrent access.
 *
 * @param p the recevied packet, p->payload pointing to the ethernet header
 * @param netif the network interface on which the packet was received
 */
err_t
ethernet_input(struct pbuf *p, struct netif *netif)
{
  struct eth_hdr* ethhdr;
  u16_t type;
#if LWIP_ARP || ETHARP_SUPPORT_VLAN
  s16_t ip_hdr_offset = SIZEOF_ETH_HDR;
#endif /* LWIP_ARP || ETHARP_SUPPORT_VLAN */
/*长度检验*/
  if (p->len <= SIZEOF_ETH_HDR) {
    /* a packet with only an ethernet header (or less) is not valid for us */
    ETHARP_STATS_INC(etharp.proterr);
    ETHARP_STATS_INC(etharp.drop);
    goto free_and_return;
  }

  /* points to packet payload, which starts with an Ethernet header */
  ethhdr = (struct eth_hdr *)p->payload;	//ethhdr指针指向以太网帧中的头部
  LWIP_DEBUGF(ETHARP_DEBUG | LWIP_DBG_TRACE,
    ("ethernet_input: dest:%"X8_F":%"X8_F":%"X8_F":%"X8_F":%"X8_F":%"X8_F", src:%"X8_F":%"X8_F":%"X8_F":%"X8_F":%"X8_F":%"X8_F", type:%"X16_F"\n",
     (unsigned)ethhdr->dest.addr[0], (unsigned)ethhdr->dest.addr[1], (unsigned)ethhdr->dest.addr[2],
     (unsigned)ethhdr->dest.addr[3], (unsigned)ethhdr->dest.addr[4], (unsigned)ethhdr->dest.addr[5],
     (unsigned)ethhdr->src.addr[0], (unsigned)ethhdr->src.addr[1], (unsigned)ethhdr->src.addr[2],
     (unsigned)ethhdr->src.addr[3], (unsigned)ethhdr->src.addr[4], (unsigned)ethhdr->src.addr[5],
     (unsigned)htons(ethhdr->type)));

  type = ethhdr->type;//获得帧类型字段
#if ETHARP_SUPPORT_VLAN
  if (type == PP_HTONS(ETHTYPE_VLAN)) {
    struct eth_vlan_hdr *vlan = (struct eth_vlan_hdr*)(((char*)ethhdr) + SIZEOF_ETH_HDR);
    if (p->len <= SIZEOF_ETH_HDR + SIZEOF_VLAN_HDR) {
      /* a packet with only an ethernet/vlan header (or less) is not valid for us */
      ETHARP_STATS_INC(etharp.proterr);
      ETHARP_STATS_INC(etharp.drop);
      goto free_and_return;
    }
#if defined(ETHARP_VLAN_CHECK) || defined(ETHARP_VLAN_CHECK_FN) /* if not, allow all VLANs */
#ifdef ETHARP_VLAN_CHECK_FN
    if (!ETHARP_VLAN_CHECK_FN(ethhdr, vlan)) {
#elif defined(ETHARP_VLAN_CHECK)
    if (VLAN_ID(vlan) != ETHARP_VLAN_CHECK) {
#endif
      /* silently ignore this packet: not for our VLAN */
      pbuf_free(p);
      return ERR_OK;
    }
#endif /* defined(ETHARP_VLAN_CHECK) || defined(ETHARP_VLAN_CHECK_FN) */
    type = vlan->tpid;
    ip_hdr_offset = SIZEOF_ETH_HDR + SIZEOF_VLAN_HDR;
  }
#endif /* ETHARP_SUPPORT_VLAN */

#if LWIP_ARP_FILTER_NETIF
  netif = LWIP_ARP_FILTER_NETIF_FN(p, netif, htons(type));
#endif /* LWIP_ARP_FILTER_NETIF*/

  if (ethhdr->dest.addr[0] & 1) {
    /* this might be a multicast or broadcast packet */
    if (ethhdr->dest.addr[0] == LL_MULTICAST_ADDR_0) {
      if ((ethhdr->dest.addr[1] == LL_MULTICAST_ADDR_1) &&
          (ethhdr->dest.addr[2] == LL_MULTICAST_ADDR_2)) {
        /* mark the pbuf as link-layer multicast */
        p->flags |= PBUF_FLAG_LLMCAST;
      }
    } else if (eth_addr_cmp(&ethhdr->dest, &ethbroadcast)) {
      /* mark the pbuf as link-layer broadcast */
      p->flags |= PBUF_FLAG_LLBCAST;
    }
  }

  switch (type) {	//判断帧类型
#if LWIP_ARP
    /* IP packet? */
    case PP_HTONS(ETHTYPE_IP)://是IP数据包
      if (!(netif->flags & NETIF_FLAG_ETHARP)) {
        goto free_and_return;
      }
#if ETHARP_TRUST_IP_MAC
      /* update ARP table */
      etharp_ip_input(netif, p);//使用IP头部以及以太网头部信息更新ARP表
#endif /* ETHARP_TRUST_IP_MAC */
      /* skip Ethernet header */
      if(pbuf_header(p, -ip_hdr_offset)) {//去掉以太网帧头部
        LWIP_ASSERT("Can't move over header in packet", 0);
        goto free_and_return;//操作失败，释放pbuf
      } else {
        /* pass to IP layer */
        ip_input(p, netif);//调用ip层输入函数，向上传递数据
      }
      break;
      
    case PP_HTONS(ETHTYPE_ARP)://arp数据包
      if (!(netif->flags & NETIF_FLAG_ETHARP)) {
        goto free_and_return;
      }
      /* pass p to ARP module */
      etharp_arp_input(netif, (struct eth_addr*)(netif->hwaddr), p);//递交给ARP模块处理
      break;
#endif /* LWIP_ARP */
#if PPPOE_SUPPORT
    case PP_HTONS(ETHTYPE_PPPOEDISC): /* PPP Over Ethernet Discovery Stage */
      pppoe_disc_input(netif, p);
      break;

    case PP_HTONS(ETHTYPE_PPPOE): /* PPP Over Ethernet Session Stage */
      pppoe_data_input(netif, p);
      break;
#endif /* PPPOE_SUPPORT */

    default://帧类型错误，直接删除数据包
      ETHARP_STATS_INC(etharp.proterr);
      ETHARP_STATS_INC(etharp.drop);
      goto free_and_return;
  }

  /* This means the pbuf is freed or consumed,
     so the caller doesn't have to free it again */
  return ERR_OK;

free_and_return://当错误发生后，释放数据包，返回ERR_OK
  pbuf_free(p);
  return ERR_OK;
}
#endif /* LWIP_ARP || LWIP_ETHERNET */
