/**
 * @file
 * This is the IPv4 packet segmentation and reassembly implementation.
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
 * Author: Jani Monoses <jani@iv.ro> 
 *         Simon Goldschmidt
 * original reassembly code by Adam Dunkels <adam@sics.se>
 * 
 */

#include "lwip/opt.h"
#include "lwip/ip_frag.h"
#include "lwip/def.h"
#include "lwip/inet_chksum.h"
#include "lwip/netif.h"
#include "lwip/snmp.h"
#include "lwip/stats.h"
#include "lwip/icmp.h"

#include <string.h>

#if IP_REASSEMBLY
/**
 * The IP reassembly code currently has the following limitations:
 * - IP header options are not supported
 * - fragments must not overlap (e.g. due to different routes),
 *   currently, overlapping or duplicate fragments are thrown away
 *   if IP_REASS_CHECK_OVERLAP=1 (the default)!
 *
 * @todo: work with IP header options
 */

/** Setting this to 0, you can turn off checking the fragments for overlapping
 * regions. The code gets a little smaller. Only use this if you know that
 * overlapping won't occur on your network! */
#ifndef IP_REASS_CHECK_OVERLAP
#define IP_REASS_CHECK_OVERLAP 1//检查分片数据包是否有重叠域
#endif /* IP_REASS_CHECK_OVERLAP */

/** Set to 0 to prevent freeing the oldest datagram when the reassembly buffer is
 * full (IP_REASS_MAX_PBUFS pbufs are enqueued). The code gets a little smaller.
 * Datagrams will be freed by timeout only. Especially useful when MEMP_NUM_REASSDATA
 * is set to 1, so one datagram can be reassembled at a time, only. */
#ifndef IP_REASS_FREE_OLDEST
#define IP_REASS_FREE_OLDEST 1
#endif /* IP_REASS_FREE_OLDEST */

#define IP_REASS_FLAG_LASTFRAG 0x01//用于ip_reassdata结构的flags字段，表示该数据报的最后一个分片是否已收到

/** This is a helper struct which holds the starting
 * offset and the ending offset of this fragment to
 * easily chain the fragments.
 * It has the same packing requirements as the IP header, since it replaces
 * the IP header in memory in incoming fragments (after copying it) to keep
 * track of the various fragments. (-> If the IP header doesn't need packing,
 * this struct doesn't need packing, too.)
 */
#ifdef PACK_STRUCT_USE_INCLUDES
#  include "arch/bpstruct.h"
#endif
//ip_reass_helper结构体定义
PACK_STRUCT_BEGIN
struct ip_reass_helper {
  PACK_STRUCT_FIELD(struct pbuf *next_pbuf);//指向下一个分片
  PACK_STRUCT_FIELD(u16_t start);//分片中数据的起始位置
  PACK_STRUCT_FIELD(u16_t end);//分片中数据的结束位置
} PACK_STRUCT_STRUCT;
PACK_STRUCT_END
#ifdef PACK_STRUCT_USE_INCLUDES
#  include "arch/epstruct.h"
#endif

#define IP_ADDRESSES_AND_ID_MATCH(iphdrA, iphdrB)  \
  (ip_addr_cmp(&(iphdrA)->src, &(iphdrB)->src) && \
   ip_addr_cmp(&(iphdrA)->dest, &(iphdrB)->dest) && \
   IPH_ID(iphdrA) == IPH_ID(iphdrB)) ? 1 : 0

/* global variables */
static struct ip_reassdata *reassdatagrams;//重装链表头指针
static u16_t ip_reass_pbufcount;//全局变量，用于记录当前所有重装结构ip_reassdata上连接的pbuf总数

/* function prototypes */
static void ip_reass_dequeue_datagram(struct ip_reassdata *ipr, struct ip_reassdata *prev);
static int ip_reass_free_complete_datagram(struct ip_reassdata *ipr, struct ip_reassdata *prev);

/**重装时间基准函数
 * Reassembly timer base function
 * for both NO_SYS == 0 and 1 (!).
 *每秒调用一次
 * Should be called every 1000 msec (defined by IP_TMR_INTERVAL).
 */
void
ip_reass_tmr(void)
{
  struct ip_reassdata *r, *prev = NULL;

  r = reassdatagrams;//指向链表头部
  while (r != NULL) {
    /* Decrement the timer. Once it reaches 0,
     * clean up the incomplete fragment assembly */
    if (r->timer > 0) {//剩余时间大于０，则将其减１
      r->timer--;
      LWIP_DEBUGF(IP_REASS_DEBUG, ("ip_reass_tmr: timer dec %"U16_F"\n",(u16_t)r->timer));
      prev = r;//指向当前重装结构体
      r = r->next;//指向下一个重装结构体
    } else {//重装结构体超时
      /* reassembly timed out */
      struct ip_reassdata *tmp;//定义临时重装结构体
      LWIP_DEBUGF(IP_REASS_DEBUG, ("ip_reass_tmr: timer timed out\n"));
      tmp = r;//指向需要释放的重装结构体
      /* get the next pointer before freeing */
      r = r->next;//指向下一个重装结构体
      /* free the helper struct and all enqueued pbufs */
      ip_reass_free_complete_datagram(tmp, prev);//释放当前结构体即其上的所有pbuf
     }
   }
}

/**
 * Free a datagram (struct ip_reassdata) and all its pbufs.
 * Updates the total count of enqueued pbufs (ip_reass_pbufcount),
 * SNMP counters and sends an ICMP time exceeded packet.
 *
 * @param ipr datagram to free
 * @param prev the previous datagram in the linked list
 * @return the number of pbufs freed
 */
static int
ip_reass_free_complete_datagram(struct ip_reassdata *ipr, struct ip_reassdata *prev)
{
  u16_t pbufs_freed = 0;
  u8_t clen;
  struct pbuf *p;
  struct ip_reass_helper *iprh;

  LWIP_ASSERT("prev != ipr", prev != ipr);
  if (prev != NULL) {
    LWIP_ASSERT("prev->next == ipr", prev->next == ipr);
  }

  snmp_inc_ipreasmfails();
#if LWIP_ICMP
  iprh = (struct ip_reass_helper *)ipr->p->payload;
  if (iprh->start == 0) {
    /* The first fragment was received, send ICMP time exceeded. */
    /* First, de-queue the first pbuf from r->p. */
    p = ipr->p;
    ipr->p = iprh->next_pbuf;
    /* Then, copy the original header into it. */
    SMEMCPY(p->payload, &ipr->iphdr, IP_HLEN);
    icmp_time_exceeded(p, ICMP_TE_FRAG);//发送一个超时ICMP报文
    clen = pbuf_clen(p);
    LWIP_ASSERT("pbufs_freed + clen <= 0xffff", pbufs_freed + clen <= 0xffff);
    pbufs_freed += clen;
    pbuf_free(p);
  }
#endif /* LWIP_ICMP */

  /* First, free all received pbufs.  The individual pbufs need to be released 
     separately as they have not yet been chained */
  p = ipr->p;
  while (p != NULL) {
    struct pbuf *pcur;
    iprh = (struct ip_reass_helper *)p->payload;
    pcur = p;
    /* get the next pointer before freeing */
    p = iprh->next_pbuf;
    clen = pbuf_clen(pcur);
    LWIP_ASSERT("pbufs_freed + clen <= 0xffff", pbufs_freed + clen <= 0xffff);
    pbufs_freed += clen;
    pbuf_free(pcur);
  }
  /* Then, unchain the struct ip_reassdata from the list and free it. */
  ip_reass_dequeue_datagram(ipr, prev);
  LWIP_ASSERT("ip_reass_pbufcount >= clen", ip_reass_pbufcount >= pbufs_freed);
  ip_reass_pbufcount -= pbufs_freed;

  return pbufs_freed;
}

#if IP_REASS_FREE_OLDEST
/**
 * Free the oldest datagram to make room for enqueueing new fragments.
 * The datagram 'fraghdr' belongs to is not freed!
 *
 * @param fraghdr IP header of the current fragment
 * @param pbufs_needed number of pbufs needed to enqueue
 *        (used for freeing other datagrams if not enough space)
 * @return the number of pbufs freed
 */
static int
ip_reass_remove_oldest_datagram(struct ip_hdr *fraghdr, int pbufs_needed)
{
  /* @todo Can't we simply remove the last datagram in the
   *       linked list behind reassdatagrams?
   */
  struct ip_reassdata *r, *oldest, *prev;
  int pbufs_freed = 0, pbufs_freed_current;
  int other_datagrams;

  /* Free datagrams until being allowed to enqueue 'pbufs_needed' pbufs,
   * but don't free the datagram that 'fraghdr' belongs to! */
  do {
    oldest = NULL;
    prev = NULL;
    other_datagrams = 0;
    r = reassdatagrams;
    while (r != NULL) {
      if (!IP_ADDRESSES_AND_ID_MATCH(&r->iphdr, fraghdr)) {
        /* Not the same datagram as fraghdr */
        other_datagrams++;
        if (oldest == NULL) {
          oldest = r;
        } else if (r->timer <= oldest->timer) {
          /* older than the previous oldest */
          oldest = r;
        }
      }
      if (r->next != NULL) {
        prev = r;
      }
      r = r->next;
    }
    if (oldest != NULL) {
      pbufs_freed_current = ip_reass_free_complete_datagram(oldest, prev);
      pbufs_freed += pbufs_freed_current;
    }
  } while ((pbufs_freed < pbufs_needed) && (other_datagrams > 1));
  return pbufs_freed;
}
#endif /* IP_REASS_FREE_OLDEST */

/**
 * Enqueues a new fragment into the fragment queue
 * @param fraghdr points to the new fragments IP hdr
 * @param clen number of pbufs needed to enqueue (used for freeing other datagrams if not enough space)
 * @return A pointer to the queue location into which the fragment was enqueued
 */
static struct ip_reassdata*
ip_reass_enqueue_new_datagram(struct ip_hdr *fraghdr, int clen)
{
  struct ip_reassdata* ipr;
  /* No matching previous fragment found, allocate a new reassdata struct */
  ipr = (struct ip_reassdata *)memp_malloc(MEMP_REASSDATA);
  if (ipr == NULL) {
#if IP_REASS_FREE_OLDEST
    if (ip_reass_remove_oldest_datagram(fraghdr, clen) >= clen) {
      ipr = (struct ip_reassdata *)memp_malloc(MEMP_REASSDATA);
    }
    if (ipr == NULL)
#endif /* IP_REASS_FREE_OLDEST */
    {
      IPFRAG_STATS_INC(ip_frag.memerr);
      LWIP_DEBUGF(IP_REASS_DEBUG,("Failed to alloc reassdata struct\n"));
      return NULL;
    }
  }
  memset(ipr, 0, sizeof(struct ip_reassdata));
  ipr->timer = IP_REASS_MAXAGE;

  /* enqueue the new structure to the front of the list */
  ipr->next = reassdatagrams;
  reassdatagrams = ipr;
  /* copy the ip header for later tests and input */
  /* @todo: no ip options supported? */
  SMEMCPY(&(ipr->iphdr), fraghdr, IP_HLEN);
  return ipr;
}

/**
 * Dequeues a datagram from the datagram queue. Doesn't deallocate the pbufs.
 * @param ipr points to the queue entry to dequeue
 */
static void
ip_reass_dequeue_datagram(struct ip_reassdata *ipr, struct ip_reassdata *prev)
{
  
  /* dequeue the reass struct  */
  if (reassdatagrams == ipr) {
    /* it was the first in the list */
    reassdatagrams = ipr->next;
  } else {
    /* it wasn't the first, so it must have a valid 'prev' */
    LWIP_ASSERT("sanity check linked list", prev != NULL);
    prev->next = ipr->next;
  }

  /* now we can free the ip_reass struct */
  memp_free(MEMP_REASSDATA, ipr);
}

/**将分片插入到ip_reassdata的重装链表，并检查是否已收到数据报的所有分片
 * Chain a new pbuf into the pbuf list that composes the datagram.  The pbuf list
 * will grow over time as  new pbufs are rx.
 * Also checks that the datagram passes basic continuity checks (if the last
 * fragment was received at least once).
 * @param root_p points to the 'root' pbuf for the current datagram being assembled.
 * @param new_p points to the pbuf for the current fragment
 * @return 0 if invalid, >0 otherwise
 */
static int
ip_reass_chain_frag_into_datagram_and_validate(struct ip_reassdata *ipr, struct pbuf *new_p)
{
  struct ip_reass_helper *iprh, *iprh_tmp, *iprh_prev=NULL;
  struct pbuf *q;
  u16_t offset,len;
  struct ip_hdr *fraghdr;
  int valid = 1;//标志是否所有分片都已收到

  /* Extract length and fragment offset from current fragment */
  fraghdr = (struct ip_hdr*)new_p->payload; //指向分片的首部区域
  len = ntohs(IPH_LEN(fraghdr)) - IPH_HL(fraghdr) * 4;//分片中数据的总长度
  offset = (ntohs(IPH_OFFSET(fraghdr)) & IP_OFFMASK) * 8;//分片在数据报中的起始位置

  //将分片首部前８个字节强制转换为ip_reass_helper结构，用于重装
  /* overwrite the fragment's ip header from the pbuf with our helper struct,
   * and setup the embedded helper structure. */
  /* make sure the struct ip_reass_helper fits into the IP header */
  LWIP_ASSERT("sizeof(struct ip_reass_helper) <= IP_HLEN",
              sizeof(struct ip_reass_helper) <= IP_HLEN);
  iprh = (struct ip_reass_helper*)new_p->payload;
  iprh->next_pbuf = NULL;//指向下一个分片结构
  iprh->start = offset;//分片中数据在整个数据报中的起始位置
  iprh->end = offset + len;//分片中数据在整个数据报中的结束位置

  //遍历ip_reassdata结构的重装,为分片查找一个合适的位置
  /* Iterate through until we either get to the end of the list (append),
   * or we find on with a larger offset (insert). */
  for (q = ipr->p; q != NULL;) {//从第一个分片开始
    iprh_tmp = (struct ip_reass_helper*)q->payload;//当前分片的ip_reass_helper结构
    if (iprh->start < iprh_tmp->start) {//如果新分片的数据起始位置更低，则找到位置了
      /* the new pbuf should be inserted before this */
      iprh->next_pbuf = q;//新分片的下一个分片指针指向q
      if (iprh_prev != NULL) {//当前分片不是第一个分片
        /* not the fragment with the lowest offset */
#if IP_REASS_CHECK_OVERLAP		//如果使能了检查重叠域检查
        if ((iprh->start < iprh_prev->end) || (iprh->end > iprh_tmp->start)) {//分片之间有重叠
          /* fragment overlaps with previous or following, throw away */
          goto freepbuf;//跳转到freepbuf删除分片，并退出
        }
#endif /* IP_REASS_CHECK_OVERLAP */
        iprh_prev->next_pbuf = new_p;//前一个分片的　next_pbuf指针指向new_p
      } else {//当前分片是第一个分片，则ip_reassdata结构的p指针指向新分片
        /* fragment with the lowest offset */
        ipr->p = new_p;
      }
      break;//插入完毕，跳出
    } else if(iprh->start == iprh_tmp->start) {//两分片数据起始位置重叠，则分片是多余的
      /* received the same datagram twice: no need to keep the datagram */
      goto freepbuf;//跳转到freepbuf删除分片，并退出
#if IP_REASS_CHECK_OVERLAP
    } else if(iprh->start < iprh_tmp->end) {//有重叠域
      /* overlap: no need to keep the new datagram */
      goto freepbuf;//跳转到freepbuf删除分片，并退出
#endif /* IP_REASS_CHECK_OVERLAP */
    } else {//到这里，说明新分片的插入位置应该在当前分片位置之后的某处
      /* Check if the fragments received so far have no wholes. */
      if (iprh_prev != NULL) {//判断当前分片与前一个分片的数据是否为连续的
        if (iprh_prev->end != iprh_tmp->start) {//不连续，说明还有分片没有收到，valid＝０
          /* There is a fragment missing between the current
           * and the previous fragment */
          valid = 0;
        }
      }
    }
    q = iprh_tmp->next_pbuf;//指向链表的下一个分片
    iprh_prev = iprh_tmp;//记录当前分片
  }
//判断插入工作是否完成，即q是否为空，若为空，将新分片插入到链表尾部
  /* If q is NULL, then we made it to the end of the list. Determine what to do now */
  if (q == NULL) {
    if (iprh_prev != NULL) {//iprh_prev已指向链表中的最后一个分片
      /* this is (for now), the fragment with the highest offset:
       * chain it to the last fragment */
#if IP_REASS_CHECK_OVERLAP
      LWIP_ASSERT("check fragments don't overlap", iprh_prev->end <= iprh->start);
#endif /* IP_REASS_CHECK_OVERLAP */
      iprh_prev->next_pbuf = new_p;//新分片插入到链表尾部
      if (iprh_prev->end != iprh->start) {//如果最后一个分片和新分片的数据不连续
        valid = 0;//还有分组未收到
      }
    } else {//链表中没有任何分片
#if IP_REASS_CHECK_OVERLAP
      LWIP_ASSERT("no previous fragment, this must be the first fragment!",
        ipr->p == NULL);
#endif /* IP_REASS_CHECK_OVERLAP */
      /* this is the first fragment we ever received for this ip datagram */
      ipr->p = new_p;//新分片成为重装链表中的第一个分片
    }
  }
//这里，判断数据报的所有分片是否已经收到
  /* At this point, the validation part begins: */
  /* If we already received the last fragment */
  if ((ipr->flags & IP_REASS_FLAG_LASTFRAG) != 0) {//先判断最后一个分片是否收到
    /* and had no wholes so far */
    if (valid) {//最有一个分片已经收到，且分片数据仍然连续
      /* then check if the rest of the fragments is here */
      /* Check if the queue starts with the first datagram */
      if (((struct ip_reass_helper*)ipr->p->payload)->start != 0) {//判断第一个分片是否收到
        valid = 0;//没收到
      } else {//第一个分片已经收到
        /* and check that there are no wholes after this datagram */
        iprh_prev = iprh;//从新分片的插入位置开始，检查其后的所有分片是否连续
        q = iprh->next_pbuf;//指向下一个分片
        while (q != NULL) {
          iprh = (struct ip_reass_helper*)q->payload;
          if (iprh_prev->end != iprh->start) {//如果不连续跳出循环
            valid = 0;
            break;
          }
          iprh_prev = iprh;//记录当前分片
          q = iprh->next_pbuf;//获得下一个分片
        }//while
        /* if still valid, all fragments are received
         * (because to the MF==0 already arrived */
        if (valid) {
          LWIP_ASSERT("sanity check", ipr->p != NULL);
          LWIP_ASSERT("sanity check",
            ((struct ip_reass_helper*)ipr->p->payload) != iprh);
          LWIP_ASSERT("validate_datagram:next_pbuf!=NULL",
            iprh->next_pbuf == NULL);
          LWIP_ASSERT("validate_datagram:datagram end!=datagram len",
            iprh->end == ipr->datagram_len);
        }
      }
    }
    /* If valid is 0 here, there are some fragments missing in the middle
     * (since MF == 0 has already arrived). Such datagrams simply time out if
     * no more fragments are received... */
    return valid;
  }
  /* If we come here, not all fragments were received, yet! */
  return 0; /* not yet valid! 数据报最后一个分片还未收到，直接返回０*/
#if IP_REASS_CHECK_OVERLAP
freepbuf:
  ip_reass_pbufcount -= pbuf_clen(new_p);
  pbuf_free(new_p);
  return 0;
#endif /* IP_REASS_CHECK_OVERLAP */
}

/**重新组合输入的IP片段成一个IP数据报
 * Reassembles incoming IP fragments into an IP datagram.
 *
 * @param p points to a pbuf chain of the fragment
 * @return NULL if reassembly is incomplete, ? otherwise
 */
struct pbuf *
ip_reass(struct pbuf *p)
{
  struct pbuf *r;
  struct ip_hdr *fraghdr;
  struct ip_reassdata *ipr;
  struct ip_reass_helper *iprh;
  u16_t offset, len;
  u8_t clen;
  struct ip_reassdata *ipr_prev = NULL;

  IPFRAG_STATS_INC(ip_frag.recv);
  snmp_inc_ipreasmreqds();

  fraghdr = (struct ip_hdr*)p->payload;//指向分片数据报首部

  if ((IPH_HL(fraghdr) * 4) != IP_HLEN) {//若首部长度不为20,释放数据包
    LWIP_DEBUGF(IP_REASS_DEBUG,("ip_reass: IP options currently not supported!\n"));
    IPFRAG_STATS_INC(ip_frag.err);
    goto nullreturn;
  }

  offset = (ntohs(IPH_OFFSET(fraghdr)) & IP_OFFMASK) * 8;//得到分片偏移量字节
  len = ntohs(IPH_LEN(fraghdr)) - IPH_HL(fraghdr) * 4;//得到分片数据长度

  /* Check if we are allowed to enqueue more datagrams. */
  clen = pbuf_clen(p);//计算分片占用的pbuf数
  if ((ip_reass_pbufcount + clen) > IP_REASS_MAX_PBUFS) {
#if IP_REASS_FREE_OLDEST
    if (!ip_reass_remove_oldest_datagram(fraghdr, clen) ||
        ((ip_reass_pbufcount + clen) > IP_REASS_MAX_PBUFS))
#endif /* IP_REASS_FREE_OLDEST */
    {
      /* No datagram could be freed and still too many pbufs enqueued */
      LWIP_DEBUGF(IP_REASS_DEBUG,("ip_reass: Overflow condition: pbufct=%d, clen=%d, MAX=%d\n",
        ip_reass_pbufcount, clen, IP_REASS_MAX_PBUFS));
      IPFRAG_STATS_INC(ip_frag.memerr);
      /* @todo: send ICMP time exceeded here? */
      /* drop this pbuf */
      goto nullreturn;
    }
  }

  /* Look for the datagram the fragment belongs to in the current datagram queue,
   * remembering the previous in the queue for later dequeueing. */
  for (ipr = reassdatagrams; ipr != NULL; ipr = ipr->next) {
    /* Check if the incoming fragment matches the one currently present
       in the reassembly buffer. If so, we proceed with copying the
       fragment into the buffer. */
    if (IP_ADDRESSES_AND_ID_MATCH(&ipr->iphdr, fraghdr)) {//若找到匹配的结构体
      LWIP_DEBUGF(IP_REASS_DEBUG, ("ip_reass: matching previous fragment ID=%"X16_F"\n",
        ntohs(IPH_ID(fraghdr))));
      IPFRAG_STATS_INC(ip_frag.cachehit);
      break;
    }
    ipr_prev = ipr;
  }

  if (ipr == NULL) {
  /* Enqueue a new datagram into the datagram queue */
    ipr = ip_reass_enqueue_new_datagram(fraghdr, clen);//为分片新建一个ip_reassdata结构体
    /* Bail if unable to enqueue */
    if(ipr == NULL) {
      goto nullreturn;
    }
  } else {
    if (((ntohs(IPH_OFFSET(fraghdr)) & IP_OFFMASK) == 0) && 
      ((ntohs(IPH_OFFSET(&ipr->iphdr)) & IP_OFFMASK) != 0)) {//看该分片是否是某个数据报的第一个分片
      /* ipr->iphdr is not the header from the first fragment, but fraghdr is
       * -> copy fraghdr into ipr->iphdr since we want to have the header
       * of the first fragment (for ICMP time exceeded and later, for copying
       * all options, if supported)*/
      SMEMCPY(&ipr->iphdr, fraghdr, IP_HLEN);//利用ip_reassdata结构体中iphdr记录首部
    }
  }
  //到这里，经过新建或查找，都为分片得到了一个ip_reassdata结构
  /* Track the current number of pbufs current 'in-flight', in order to limit 
  the number of fragments that may be enqueued at any one time */
  ip_reass_pbufcount += clen;

  /* At this point, we have either created a new entry or pointing 
   * to an existing one */
//查看是否是最后一个分片，是的话IP_MF＝０
  /* check for 'no more fragments', and update queue entry*/
  if ((IPH_OFFSET(fraghdr) & PP_NTOHS(IP_MF)) == 0) {
    ipr->flags |= IP_REASS_FLAG_LASTFRAG;
    ipr->datagram_len = offset + len;
    LWIP_DEBUGF(IP_REASS_DEBUG,
     ("ip_reass: last fragment seen, total len %"S16_F"\n",
      ipr->datagram_len));
  }
  //寻找合适的位置插入分片数据包
  /* find the right place to insert this pbuf */
  /* @todo: trim pbufs if fragments are overlapping */
  if (ip_reass_chain_frag_into_datagram_and_validate(ipr, p)) {//若所有分片已经到达，若没有全部直接不进行任何操作
    /* the totally last fragment (flag more fragments = 0) was received at least
     * once AND all fragments are received */
    ipr->datagram_len += IP_HLEN;//datagram_len设置为数据报总长度

    /* save the second pbuf before copying the header over the pointer */
    r = ((struct ip_reass_helper*)ipr->p->payload)->next_pbuf;//r指向第二个分片pbuf

    /* copy the original ip header back to the first pbuf */
    fraghdr = (struct ip_hdr*)(ipr->p->payload);//指向第一个分片的首部
    SMEMCPY(fraghdr, &ipr->iphdr, IP_HLEN);//将ip首部拷贝到第一个分片中
    IPH_LEN_SET(fraghdr, htons(ipr->datagram_len));//设置总长度字段
    IPH_OFFSET_SET(fraghdr, 0);//ip分片相关的字段全部清０
    IPH_CHKSUM_SET(fraghdr, 0);//校验和字段清０
    /* @todo: do we need to set calculate the correct checksum? */
    IPH_CHKSUM_SET(fraghdr, inet_chksum(fraghdr, IP_HLEN));//计算填写校验和

    p = ipr->p;//指向第一个分片，即重装后的数据报起始pbuf

    /* chain together the pbufs contained within the reass_data list. */
    while(r != NULL) {//从第二个分片开始，调整各个分片pbuf的payload指针
      iprh = (struct ip_reass_helper*)r->payload;

      /* hide the ip header for every succeding fragment */
      pbuf_header(r, -IP_HLEN);//调整ayload指针，隐藏分片中的ip首部
      pbuf_cat(p, r);//连接两个分片的pbuf
      r = iprh->next_pbuf;//r指向下一个分片起始pbuf
    }
    //到这里，整个数据报的重装工作就完成了，需要删除链表reassdatagrams中与数据报对应的ip_reassdata结构，并将数据包pbuf指针返回
    /* release the sources allocate for the fragment queue entry */
    ip_reass_dequeue_datagram(ipr, ipr_prev);//从链表中删除ip_reassdata结构

    /* and adjust the number of pbufs currently queued for reassembly. */
    ip_reass_pbufcount -= pbuf_clen(p);//减少全局变量值

    /* Return the pbuf chain */
    return p;
  }
  /* the datagram is not (yet?) reassembled completely */
  LWIP_DEBUGF(IP_REASS_DEBUG,("ip_reass_pbufcount: %d out\n", ip_reass_pbufcount));
  return NULL;

nullreturn:
  LWIP_DEBUGF(IP_REASS_DEBUG,("ip_reass: nullreturn\n"));
  IPFRAG_STATS_INC(ip_frag.drop);
  pbuf_free(p);
  return NULL;
}
#endif /* IP_REASSEMBLY */

#if IP_FRAG
#if IP_FRAG_USES_STATIC_BUF
//定义一个全局型的数组，数组大小为ip层运行的最大分片大小，每个分片会被先后拷贝到这个数组中，然后发送
static u8_t buf[LWIP_MEM_ALIGN_SIZE(IP_FRAG_MAX_MTU + MEM_ALIGNMENT - 1)];
#else /* IP_FRAG_USES_STATIC_BUF */

#if !LWIP_NETIF_TX_SINGLE_PBUF
/** Allocate a new struct pbuf_custom_ref */
static struct pbuf_custom_ref*
ip_frag_alloc_pbuf_custom_ref(void)
{
  return (struct pbuf_custom_ref*)memp_malloc(MEMP_FRAG_PBUF);
}

/** Free a struct pbuf_custom_ref */
static void
ip_frag_free_pbuf_custom_ref(struct pbuf_custom_ref* p)
{
  LWIP_ASSERT("p != NULL", p != NULL);
  memp_free(MEMP_FRAG_PBUF, p);
}

/** Free-callback function to free a 'struct pbuf_custom_ref', called by
 * pbuf_free. */
static void
ipfrag_free_pbuf_custom(struct pbuf *p)
{
  struct pbuf_custom_ref *pcr = (struct pbuf_custom_ref*)p;
  LWIP_ASSERT("pcr != NULL", pcr != NULL);
  LWIP_ASSERT("pcr == p", (void*)pcr == (void*)p);
  if (pcr->original != NULL) {
    pbuf_free(pcr->original);
  }
  ip_frag_free_pbuf_custom_ref(pcr);
}
#endif /* !LWIP_NETIF_TX_SINGLE_PBUF */
#endif /* IP_FRAG_USES_STATIC_BUF */

/**将数据报p进行分片发送
 * Fragment an IP datagram if too large for the netif.
 *
 * Chop the datagram in MTU sized chunks and send them in order
 * by using a fixed size static memory buffer (PBUF_REF) or
 * point PBUF_REFs into p (depending on IP_FRAG_USES_STATIC_BUF).
 *
 * @param p ip packet to send
 * @param netif the netif on which to send
 * @param dest destination ip address to which to send
 *
 * @return ERR_OK if sent successfully, err_t otherwise
 */
err_t 
ip_frag(struct pbuf *p, struct netif *netif, ip_addr_t *dest)
{
  struct pbuf *rambuf;			//分片的pbuf结构
#if IP_FRAG_USES_STATIC_BUF
  struct pbuf *header;			//以太网帧pbuf
#else
#if !LWIP_NETIF_TX_SINGLE_PBUF
  struct pbuf *newpbuf;
#endif
  struct ip_hdr *original_iphdr;
#endif
  struct ip_hdr *iphdr;			//ip首部指针
  u16_t nfb;					//分片中允许的最大数据量
  u16_t left, cop;				//待发送数据长度和当前发送的数据长度
  u16_t mtu = netif->mtu;		//网络接口mtu
  u16_t ofo, omf;				//分片偏移量和更多分片位
  u16_t last;					//是否为最后一个分片
  u16_t poff = IP_HLEN;			//发送的数据在原始数据报pbuf中的个数
  u16_t tmp;
#if !IP_FRAG_USES_STATIC_BUF && !LWIP_NETIF_TX_SINGLE_PBUF
  u16_t newpbuflen = 0;
  u16_t left_to_copy;
#endif

  /* Get a RAM based MTU sized pbuf */
#if IP_FRAG_USES_STATIC_BUF
  /* When using a static buffer, we use a PBUF_REF, which we will
   * use to reference the packet (without link header).
   * Layer and length is irrelevant.
   */
  rambuf = pbuf_alloc(PBUF_LINK, 0, PBUF_REF);		//为数据分片申请一个pubf结构
  if (rambuf == NULL) {
    LWIP_DEBUGF(IP_REASS_DEBUG, ("ip_frag: pbuf_alloc(PBUF_LINK, 0, PBUF_REF) failed\n"));
    return ERR_MEM;//申请失败，返回
  }
  rambuf->tot_len = rambuf->len = mtu;//设置pbuf的len和tot_len字段为接口的mtu值
  rambuf->payload = LWIP_MEM_ALIGN((void *)buf);//payload指向全局数据区域，得到分片包存储区域

  /* Copy the IP header in it */
  iphdr = (struct ip_hdr *)rambuf->payload;//ip首部指向分片包区域
  SMEMCPY(iphdr, p->payload, IP_HLEN);//把原始数据报首部拷贝到分片的首部
#else /* IP_FRAG_USES_STATIC_BUF */
  original_iphdr = (struct ip_hdr *)p->payload;
  iphdr = original_iphdr;
#endif /* IP_FRAG_USES_STATIC_BUF */

  /* Save original offset */
  tmp = ntohs(IPH_OFFSET(iphdr));//暂存分片的相关字段（由网络序转换成主机序）
  ofo = tmp & IP_OFFMASK;//得到分片偏移量（对原始数据报为０）
  omf = tmp & IP_MF;//得到更多分片标志值

  left = p->tot_len - IP_HLEN;//待发送数据长度（总长度－ip首部长度）

  nfb = (mtu - IP_HLEN) / 8;//一个分片中可以存放的最大数据量（８字节为单位）

  while (left) {			//待发送数据长度大于０
    last = (left <= mtu - IP_HLEN);			//若待发送长度小于分片最大长度，last为１，否则为０

    /* Set new offset and MF flag */
    tmp = omf | (IP_OFFMASK & (ofo));//计算分片相关字段
    if (!last) {//如果不是最后一片
      tmp = tmp | IP_MF;//则更多分片位置为１
    }

    /* Fill this fragment */
    cop = last ? left : nfb * 8;//计算当前分片中数据长度，，当last为１时说名分片能装下剩余数据，否则进行分片

#if IP_FRAG_USES_STATIC_BUF
    //从原始数据报中拷贝cop字节的数据到分片中，poff记录了拷贝起始位置
    poff += pbuf_copy_partial(p, (u8_t*)iphdr + IP_HLEN, cop, poff);
#else /* IP_FRAG_USES_STATIC_BUF */
#if LWIP_NETIF_TX_SINGLE_PBUF
    rambuf = pbuf_alloc(PBUF_IP, cop, PBUF_RAM);
    if (rambuf == NULL) {
      return ERR_MEM;
    }
    LWIP_ASSERT("this needs a pbuf in one piece!",
      (rambuf->len == rambuf->tot_len) && (rambuf->next == NULL));
    poff += pbuf_copy_partial(p, rambuf->payload, cop, poff);
    /* make room for the IP header */
    if(pbuf_header(rambuf, IP_HLEN)) {
      pbuf_free(rambuf);
      return ERR_MEM;
    }
    /* fill in the IP header */
    SMEMCPY(rambuf->payload, original_iphdr, IP_HLEN);
    iphdr = rambuf->payload;
#else /* LWIP_NETIF_TX_SINGLE_PBUF */
    /* When not using a static buffer, create a chain of pbufs.
     * The first will be a PBUF_RAM holding the link and IP header.
     * The rest will be PBUF_REFs mirroring the pbuf chain to be fragged,
     * but limited to the size of an mtu.
     */
    rambuf = pbuf_alloc(PBUF_LINK, IP_HLEN, PBUF_RAM);
    if (rambuf == NULL) {
      return ERR_MEM;
    }
    LWIP_ASSERT("this needs a pbuf in one piece!",
                (p->len >= (IP_HLEN)));
    SMEMCPY(rambuf->payload, original_iphdr, IP_HLEN);
    iphdr = (struct ip_hdr *)rambuf->payload;

    /* Can just adjust p directly for needed offset. */
    p->payload = (u8_t *)p->payload + poff;
    p->len -= poff;

    left_to_copy = cop;
    while (left_to_copy) {
      struct pbuf_custom_ref *pcr;
      newpbuflen = (left_to_copy < p->len) ? left_to_copy : p->len;
      /* Is this pbuf already empty? */
      if (!newpbuflen) {
        p = p->next;
        continue;
      }
      pcr = ip_frag_alloc_pbuf_custom_ref();
      if (pcr == NULL) {
        pbuf_free(rambuf);
        return ERR_MEM;
      }
      /* Mirror this pbuf, although we might not need all of it. */
      newpbuf = pbuf_alloced_custom(PBUF_RAW, newpbuflen, PBUF_REF, &pcr->pc, p->payload, newpbuflen);
      if (newpbuf == NULL) {
        ip_frag_free_pbuf_custom_ref(pcr);
        pbuf_free(rambuf);
        return ERR_MEM;
      }
      pbuf_ref(p);
      pcr->original = p;
      pcr->pc.custom_free_function = ipfrag_free_pbuf_custom;

      /* Add it to end of rambuf's chain, but using pbuf_cat, not pbuf_chain
       * so that it is removed when pbuf_dechain is later called on rambuf.
       */
      pbuf_cat(rambuf, newpbuf);
      left_to_copy -= newpbuflen;
      if (left_to_copy) {
        p = p->next;
      }
    }
    poff = newpbuflen;
#endif /* LWIP_NETIF_TX_SINGLE_PBUF */
#endif /* IP_FRAG_USES_STATIC_BUF */
//填写分片首部中的其他字段
    /* Correct header */
    IPH_OFFSET_SET(iphdr, htons(tmp));//填写分片相关字段
    IPH_LEN_SET(iphdr, htons(cop + IP_HLEN));//填写总长度
    IPH_CHKSUM_SET(iphdr, 0);//清零校验和
    IPH_CHKSUM_SET(iphdr, inet_chksum(iphdr, IP_HLEN));//计算校验和

#if IP_FRAG_USES_STATIC_BUF
    if (last) {//如果还有分片，将pbuf收缩到所需长度
      pbuf_realloc(rambuf, left + IP_HLEN);
    }

    /* This part is ugly: we alloc a RAM based pbuf for 
     * the link level header for each chunk and then 
     * free it.A PBUF_ROM style pbuf for which pbuf_header
     * worked would make things simpler.
     */
    //把组装好的分片发送出去，重新在内存堆中开辟一个pbuf空间，用来保存以太网帧首部
    header = pbuf_alloc(PBUF_LINK, 0, PBUF_RAM);//申请分配一个pbuf结构＋以太网帧头部空间
    if (header != NULL) {
      pbuf_chain(header, rambuf);//将两个pbuf连接成一个pbuf链表
      netif->output(netif, header, dest);//调用网络输出口发送数据包
      IPFRAG_STATS_INC(ip_frag.xmit);
      snmp_inc_ipfragcreates();//网络管理方面内容
      pbuf_free(header);//发送完成后，释放pbuf链表
    } else {//若以太网首部空间申请失败
      LWIP_DEBUGF(IP_REASS_DEBUG, ("ip_frag: pbuf_alloc() for header failed\n"));
      pbuf_free(rambuf);//释放分片空间
      return ERR_MEM;//返回内存错误
    }
#else /* IP_FRAG_USES_STATIC_BUF */
    /* No need for separate header pbuf - we allowed room for it in rambuf
     * when allocated.
     */
    netif->output(netif, rambuf, dest);
    IPFRAG_STATS_INC(ip_frag.xmit);

    /* Unfortunately we can't reuse rambuf - the hardware may still be
     * using the buffer. Instead we free it (and the ensuing chain) and
     * recreate it next time round the loop. If we're lucky the hardware
     * will have already sent the packet, the free will really free, and
     * there will be zero memory penalty.
     */
    
    pbuf_free(rambuf);
#endif /* IP_FRAG_USES_STATIC_BUF */
    left -= cop;//待发送的总长度减少
    ofo += nfb;//分片偏移量增加
  }//while
#if IP_FRAG_USES_STATIC_BUF
  pbuf_free(rambuf);//释放结构rambuf
#endif /* IP_FRAG_USES_STATIC_BUF */
  snmp_inc_ipfragoks();
  return ERR_OK;
}
#endif /* IP_FRAG */
