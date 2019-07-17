/**
 * @file
 * Dynamic pool memory manager
 *
 * lwIP has dedicated pools for many structures (netconn, protocol control blocks,
 * packet buffers, ...). All these pools are managed here.
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

#include "lwip/opt.h"

#include "lwip/memp.h"
#include "lwip/pbuf.h"
#include "lwip/udp.h"
#include "lwip/raw.h"
#include "lwip/tcp_impl.h"
#include "lwip/igmp.h"
#include "lwip/api.h"
#include "lwip/api_msg.h"
#include "lwip/tcpip.h"
#include "lwip/sys.h"
#include "lwip/timers.h"
#include "lwip/stats.h"
#include "netif/etharp.h"
#include "lwip/ip_frag.h"
#include "lwip/snmp_structs.h"
#include "lwip/snmp_msg.h"
#include "lwip/dns.h"
#include "netif/ppp_oe.h"

#include <string.h>

#if !MEMP_MEM_MALLOC /* don't build if not configured for use in lwipopts.h */

struct memp {
  struct memp *next;
#if MEMP_OVERFLOW_CHECK
  const char *file;
  int line;
#endif /* MEMP_OVERFLOW_CHECK */
};

#if MEMP_OVERFLOW_CHECK
/* if MEMP_OVERFLOW_CHECK is turned on, we reserve some bytes at the beginning
 * and at the end of each element, initialize them as 0xcd and check
 * them later. */
/* If MEMP_OVERFLOW_CHECK is >= 2, on every call to memp_malloc or memp_free,
 * every single element in each pool is checked!
 * This is VERY SLOW but also very helpful. */
/* MEMP_SANITY_REGION_BEFORE and MEMP_SANITY_REGION_AFTER can be overridden in
 * lwipopts.h to change the amount reserved for checking. */
#ifndef MEMP_SANITY_REGION_BEFORE
#define MEMP_SANITY_REGION_BEFORE  16
#endif /* MEMP_SANITY_REGION_BEFORE*/
#if MEMP_SANITY_REGION_BEFORE > 0
#define MEMP_SANITY_REGION_BEFORE_ALIGNED    LWIP_MEM_ALIGN_SIZE(MEMP_SANITY_REGION_BEFORE)
#else
#define MEMP_SANITY_REGION_BEFORE_ALIGNED    0
#endif /* MEMP_SANITY_REGION_BEFORE*/
#ifndef MEMP_SANITY_REGION_AFTER
#define MEMP_SANITY_REGION_AFTER   16
#endif /* MEMP_SANITY_REGION_AFTER*/
#if MEMP_SANITY_REGION_AFTER > 0
#define MEMP_SANITY_REGION_AFTER_ALIGNED     LWIP_MEM_ALIGN_SIZE(MEMP_SANITY_REGION_AFTER)
#else
#define MEMP_SANITY_REGION_AFTER_ALIGNED     0
#endif /* MEMP_SANITY_REGION_AFTER*/

/* MEMP_SIZE: save space for struct memp and for sanity check */
#define MEMP_SIZE          (LWIP_MEM_ALIGN_SIZE(sizeof(struct memp)) + MEMP_SANITY_REGION_BEFORE_ALIGNED)
#define MEMP_ALIGN_SIZE(x) (LWIP_MEM_ALIGN_SIZE(x) + MEMP_SANITY_REGION_AFTER_ALIGNED)

#else /* MEMP_OVERFLOW_CHECK */

/* No sanity checks
 * We don't need to preserve the struct memp while not allocated, so we
 * can save a little space and set MEMP_SIZE to 0.
 */
#define MEMP_SIZE           0
#define MEMP_ALIGN_SIZE(x) (LWIP_MEM_ALIGN_SIZE(x))

#endif /* MEMP_OVERFLOW_CHECK */

/** This array holds the first free element of each pool.
 *  Elements form a linked list. */
static struct memp *memp_tab[MEMP_MAX];//全局指针数组，指向每类POOL中第一个POOL

#else /* MEMP_MEM_MALLOC */

#define MEMP_ALIGN_SIZE(x) (LWIP_MEM_ALIGN_SIZE(x))

#endif /* MEMP_MEM_MALLOC */

/** This array holds the element sizes of each pool. */
#if !MEM_USE_POOLS && !MEMP_MEM_MALLOC
static
#endif
const u16_t memp_sizes[MEMP_MAX] = {
#define LWIP_MEMPOOL(name,num,size,desc)  LWIP_MEM_ALIGN_SIZE(size),
#include "lwip/memp_std.h"
};//全局数组，每类POOL中单个POOL的大小

#if !MEMP_MEM_MALLOC /* don't build if not configured for use in lwipopts.h */

/** This array holds the number of elements in each pool. */
static const u16_t memp_num[MEMP_MAX] = {
#define LWIP_MEMPOOL(name,num,size,desc)  (num),
#include "lwip/memp_std.h"
};//全局数组，每类POOL中POOL的个数

/** This array holds a textual description of each pool. */
#ifdef LWIP_DEBUG
static const char *memp_desc[MEMP_MAX] = {
#define LWIP_MEMPOOL(name,num,size,desc)  (desc),
#include "lwip/memp_std.h"
};//全局指针数组，指向每类POOL的描述字符串
#endif /* LWIP_DEBUG */

#if MEMP_SEPARATE_POOLS

/** This creates each memory pool. These are named memp_memory_XXX_base (where
 * XXX is the name of the pool defined in memp_std.h).
 * To relocate a pool, declare it as extern in cc.h. Example for GCC:
 *   extern u8_t __attribute__((section(".onchip_mem"))) memp_memory_UDP_PCB_base[];
 */
#define LWIP_MEMPOOL(name,num,size,desc) u8_t memp_memory_ ## name ## _base \
  [((num) * (MEMP_SIZE + MEMP_ALIGN_SIZE(size)))];   
#include "lwip/memp_std.h"

/** This array holds the base of each memory pool. */
static u8_t *const memp_bases[] = { 
#define LWIP_MEMPOOL(name,num,size,desc) memp_memory_ ## name ## _base,   
#include "lwip/memp_std.h"
};

#else /* MEMP_SEPARATE_POOLS */

/** This is the actual memory used by the pools (all pools in one big block). */
static u8_t memp_memory[MEM_ALIGNMENT - 1 
#define LWIP_MEMPOOL(name,num,size,desc) + ( (num) * (MEMP_SIZE + MEMP_ALIGN_SIZE(size) ) )
#include "lwip/memp_std.h"
];//全局数组，为所有POOL分配的内存空间

#endif /* MEMP_SEPARATE_POOLS */

#if MEMP_SANITY_CHECK
/**
 * Check that memp-lists don't form a circle, using "Floyd's cycle-finding algorithm".
 */
static int
memp_sanity(void)
{
  s16_t i;
  struct memp *t, *h;

  for (i = 0; i < MEMP_MAX; i++) {
    t = memp_tab[i];
    if(t != NULL) {
      for (h = t->next; (t != NULL) && (h != NULL); t = t->next,
        h = (((h->next != NULL) && (h->next->next != NULL)) ? h->next->next : NULL)) {
        if (t == h) {
          return 0;
        }
      }
    }
  }
  return 1;
}
#endif /* MEMP_SANITY_CHECK*/
#if MEMP_OVERFLOW_CHECK
#if defined(LWIP_DEBUG) && MEMP_STATS
static const char * memp_overflow_names[] = {
#define LWIP_MEMPOOL(name,num,size,desc) "/"desc,
#include "lwip/memp_std.h"
  };
#endif

/**
 * Check if a memp element was victim of an overflow
 * (e.g. the restricted area after it has been altered)
 *
 * @param p the memp element to check
 * @param memp_type the pool p comes from
 */
static void
memp_overflow_check_element_overflow(struct memp *p, u16_t memp_type)
{
  u16_t k;
  u8_t *m;
#if MEMP_SANITY_REGION_AFTER_ALIGNED > 0
  m = (u8_t*)p + MEMP_SIZE + memp_sizes[memp_type];
  for (k = 0; k < MEMP_SANITY_REGION_AFTER_ALIGNED; k++) {
    if (m[k] != 0xcd) {
      char errstr[128] = "detected memp overflow in pool ";
      char digit[] = "0";
      if(memp_type >= 10) {
        digit[0] = '0' + (memp_type/10);
        strcat(errstr, digit);
      }
      digit[0] = '0' + (memp_type%10);
      strcat(errstr, digit);
#if defined(LWIP_DEBUG) && MEMP_STATS
      strcat(errstr, memp_overflow_names[memp_type]);
#endif
      LWIP_ASSERT(errstr, 0);
    }
  }
#endif
}

/**
 * Check if a memp element was victim of an underflow
 * (e.g. the restricted area before it has been altered)
 *
 * @param p the memp element to check
 * @param memp_type the pool p comes from
 */
static void
memp_overflow_check_element_underflow(struct memp *p, u16_t memp_type)
{
  u16_t k;
  u8_t *m;
#if MEMP_SANITY_REGION_BEFORE_ALIGNED > 0
  m = (u8_t*)p + MEMP_SIZE - MEMP_SANITY_REGION_BEFORE_ALIGNED;
  for (k = 0; k < MEMP_SANITY_REGION_BEFORE_ALIGNED; k++) {
    if (m[k] != 0xcd) {
      char errstr[128] = "detected memp underflow in pool ";
      char digit[] = "0";
      if(memp_type >= 10) {
        digit[0] = '0' + (memp_type/10);
        strcat(errstr, digit);
      }
      digit[0] = '0' + (memp_type%10);
      strcat(errstr, digit);
#if defined(LWIP_DEBUG) && MEMP_STATS
      strcat(errstr, memp_overflow_names[memp_type]);
#endif
      LWIP_ASSERT(errstr, 0);
    }
  }
#endif
}

/**
 * Do an overflow check for all elements in every pool.
 *
 * @see memp_overflow_check_element for a description of the check
 */
static void
memp_overflow_check_all(void)
{
  u16_t i, j;
  struct memp *p;

  p = (struct memp *)LWIP_MEM_ALIGN(memp_memory);
  for (i = 0; i < MEMP_MAX; ++i) {
    p = p;
    for (j = 0; j < memp_num[i]; ++j) {
      memp_overflow_check_element_overflow(p, i);
      p = (struct memp*)((u8_t*)p + MEMP_SIZE + memp_sizes[i] + MEMP_SANITY_REGION_AFTER_ALIGNED);
    }
  }
  p = (struct memp *)LWIP_MEM_ALIGN(memp_memory);
  for (i = 0; i < MEMP_MAX; ++i) {
    p = p;
    for (j = 0; j < memp_num[i]; ++j) {
      memp_overflow_check_element_underflow(p, i);
      p = (struct memp*)((u8_t*)p + MEMP_SIZE + memp_sizes[i] + MEMP_SANITY_REGION_AFTER_ALIGNED);
    }
  }
}

/**
 * Initialize the restricted areas of all memp elements in every pool.
 */
static void
memp_overflow_init(void)
{
  u16_t i, j;
  struct memp *p;
  u8_t *m;

  p = (struct memp *)LWIP_MEM_ALIGN(memp_memory);
  for (i = 0; i < MEMP_MAX; ++i) {
    p = p;
    for (j = 0; j < memp_num[i]; ++j) {
#if MEMP_SANITY_REGION_BEFORE_ALIGNED > 0
      m = (u8_t*)p + MEMP_SIZE - MEMP_SANITY_REGION_BEFORE_ALIGNED;
      memset(m, 0xcd, MEMP_SANITY_REGION_BEFORE_ALIGNED);
#endif
#if MEMP_SANITY_REGION_AFTER_ALIGNED > 0
      m = (u8_t*)p + MEMP_SIZE + memp_sizes[i];
      memset(m, 0xcd, MEMP_SANITY_REGION_AFTER_ALIGNED);
#endif
      p = (struct memp*)((u8_t*)p + MEMP_SIZE + memp_sizes[i] + MEMP_SANITY_REGION_AFTER_ALIGNED);
    }
  }
}
#endif /* MEMP_OVERFLOW_CHECK */

/**
 * Initialize this module.
 * 
 * Carves out memp_memory into linked lists for each pool-type.
 */
void
memp_init(void)
{
  struct memp *memp;
  u16_t i, j;

  for (i = 0; i < MEMP_MAX; ++i) {
    MEMP_STATS_AVAIL(used, i, 0);
    MEMP_STATS_AVAIL(max, i, 0);
    MEMP_STATS_AVAIL(err, i, 0);
    MEMP_STATS_AVAIL(avail, i, memp_num[i]);
  }

#if !MEMP_SEPARATE_POOLS
  memp = (struct memp *)LWIP_MEM_ALIGN(memp_memory);//将内存池起始空间进行对齐
#endif /* !MEMP_SEPARATE_POOLS */
  /* for every pool: */
  for (i = 0; i < MEMP_MAX; ++i) {//依次对每种类型的POOL进行操作
    memp_tab[i] = NULL;//初始指针为空
#if MEMP_SEPARATE_POOLS
    memp = (struct memp*)memp_bases[i];
#endif /* MEMP_SEPARATE_POOLS */
    /* create a linked list of memp elements */
    for (j = 0; j < memp_num[i]; ++j) {//将该类型所有POLL组成链表的形式
      memp->next = memp_tab[i];
      memp_tab[i] = memp;
      memp = (struct memp *)(void *)((u8_t *)memp + MEMP_SIZE + memp_sizes[i]
#if MEMP_OVERFLOW_CHECK
        + MEMP_SANITY_REGION_AFTER_ALIGNED
#endif
      );//将每个POOL的起始处转换为memp类型，以实现链表连接
    }
  }
#if MEMP_OVERFLOW_CHECK
  memp_overflow_init();
  /* check everything a first time to see if it worked */
  memp_overflow_check_all();
#endif /* MEMP_OVERFLOW_CHECK */
}

/**
 * Get an element from a specific pool.
 *
 * @param type the pool to get an element from
 *
 * the debug version has two more parameters:
 * @param file file name calling this function
 * @param line number of line where this function is called
 *
 * @return a pointer to the allocated memory or a NULL pointer on error
 */
void *
#if !MEMP_OVERFLOW_CHECK
memp_malloc(memp_t type)//输入参数为需要分配pool类型
#else
memp_malloc_fn(memp_t type, const char* file, const int line)
#endif
{
  struct memp *memp;
  SYS_ARCH_DECL_PROTECT(old_level);//声明一个临界区保护变量
 
  LWIP_ERROR("memp_malloc: type < MEMP_MAX", (type < MEMP_MAX), return NULL;);

  SYS_ARCH_PROTECT(old_level);//进入临界区
#if MEMP_OVERFLOW_CHECK >= 2
  memp_overflow_check_all();
#endif /* MEMP_OVERFLOW_CHECK >= 2 */

  memp = memp_tab[type];//获得对应头指针指向的POOL
  
  if (memp != NULL) {
    memp_tab[type] = memp->next;//头指针指向下一个节点
#if MEMP_OVERFLOW_CHECK
    memp->next = NULL;
    memp->file = file;
    memp->line = line;
#endif /* MEMP_OVERFLOW_CHECK */
    MEMP_STATS_INC_USED(used, type);//增加内存池分配相关的统计量
    LWIP_ASSERT("memp_malloc: memp properly aligned",
                ((mem_ptr_t)memp % MEM_ALIGNMENT) == 0);
    memp = (struct memp*)(void *)((u8_t*)memp + MEMP_SIZE);//留出预留空间，这里MEMP_SIZE为０，不预留任何空间
  } else {
    LWIP_DEBUGF(MEMP_DEBUG | LWIP_DBG_LEVEL_SERIOUS, ("memp_malloc: out of memory in pool %s\n", memp_desc[type]));
    MEMP_STATS_INC(err, type);//增加内存池分配出错统计量
  }

  SYS_ARCH_UNPROTECT(old_level);//退出临界区

  return memp;//返回可用空间起始地址
}

/**
 * Put an element back into its pool.
 *
 * @param type the pool where to put mem
 * @param mem the memp element to free
 */
void
memp_free(memp_t type, void *mem)//两个参数，释放的POOL的类型和起始地址
{
  struct memp *memp;
  SYS_ARCH_DECL_PROTECT(old_level);//声明临界区保护变量

  if (mem == NULL) {//释放地址如果为空，直接返回
    return;
  }
  LWIP_ASSERT("memp_free: mem properly aligned",
                ((mem_ptr_t)mem % MEM_ALIGNMENT) == 0);

  memp = (struct memp *)(void *)((u8_t*)mem - MEMP_SIZE);//得到POOL的起始地址

  SYS_ARCH_PROTECT(old_level);//进入临界区
#if MEMP_OVERFLOW_CHECK
#if MEMP_OVERFLOW_CHECK >= 2
  memp_overflow_check_all();
#else
  memp_overflow_check_element_overflow(memp, type);
  memp_overflow_check_element_underflow(memp, type);
#endif /* MEMP_OVERFLOW_CHECK >= 2 */
#endif /* MEMP_OVERFLOW_CHECK */

  MEMP_STATS_DEC(used, type); //减少内存池分配相关的统计量
  
  memp->next = memp_tab[type]; //将POOL插入到memp_tab[type]头部
  memp_tab[type] = memp;//更改头部指针

#if MEMP_SANITY_CHECK
  LWIP_ASSERT("memp sanity", memp_sanity());
#endif /* MEMP_SANITY_CHECK */

  SYS_ARCH_UNPROTECT(old_level);//退出临界区
}

#endif /* MEMP_MEM_MALLOC */
