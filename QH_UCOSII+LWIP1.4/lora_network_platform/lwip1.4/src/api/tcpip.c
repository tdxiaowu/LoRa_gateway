/**
 * @file
 * Sequential API Main thread module
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

#include "lwip/opt.h"

#if !NO_SYS /* don't build if not configured for use in lwipopts.h */

#include "lwip/sys.h"
#include "lwip/memp.h"
#include "lwip/mem.h"
#include "lwip/pbuf.h"
#include "lwip/tcpip.h"
#include "lwip/init.h"
#include "netif/etharp.h"
#include "netif/ppp_oe.h"

/* global variables */
static tcpip_init_done_fn tcpip_init_done;//用户可注册的初始化函数
static void *tcpip_init_done_arg;//用户参数
static sys_mbox_t mbox;//定义一个全局邮箱，该邮箱为内核进程接收消息的邮箱

#if LWIP_TCPIP_CORE_LOCKING
/** The global semaphore to lock the stack. */
sys_mutex_t lock_tcpip_core;
#endif /* LWIP_TCPIP_CORE_LOCKING */


/**内核进程，识别消息的类型，上传消息
 * The main lwIP thread. This thread has exclusive access to lwIP core functions
 * (unless access to them is not locked). Other threads communicate with this
 * thread using message boxes.
 *
 * It also starts all the timers to make sure they are running in the right
 * thread context.
 *
 * @param arg unused argument
 */
static void
tcpip_thread(void *arg)
{
  struct tcpip_msg *msg;//消息结构指针
  LWIP_UNUSED_ARG(arg);//

  if (tcpip_init_done != NULL) {//若用户注册了自定义初始化函数，则调用
    tcpip_init_done(tcpip_init_done_arg);
  }

  LOCK_TCPIP_CORE();
  while (1) {                          /* MAIN Loop */
    UNLOCK_TCPIP_CORE();
    LWIP_TCPIP_THREAD_ALIVE();
    /* wait for a message, timeouts are processed while waiting */
    sys_timeouts_mbox_fetch(&mbox, (void **)&msg);//阻塞邮箱，等待一个消息
    LOCK_TCPIP_CORE();
    switch (msg->type) {//根据消息的不同类型做处理
#if LWIP_NETCONN
    case TCPIP_MSG_API://API调用
      LWIP_DEBUGF(TCPIP_DEBUG, ("tcpip_thread: API message %p\n", (void *)msg));
      msg->msg.apimsg->function(&(msg->msg.apimsg->msg));//执行API函数do_xxxxx
      break;
#endif /* LWIP_NETCONN */

#if !LWIP_TCPIP_CORE_LOCKING_INPUT
    case TCPIP_MSG_INPKT://底层数据包输入
      LWIP_DEBUGF(TCPIP_DEBUG, ("tcpip_thread: PACKET %p\n", (void *)msg));
#if LWIP_ETHERNET
      if (msg->msg.inp.netif->flags & (NETIF_FLAG_ETHARP | NETIF_FLAG_ETHERNET)) {//若支持ARP,则交给arp进行
        ethernet_input(msg->msg.inp.p, msg->msg.inp.netif);//消息进入apr
      } else
#endif /* LWIP_ETHERNET */
      {//若接口不支持ARP，则数据包直接交给IP层处理
        ip_input(msg->msg.inp.p, msg->msg.inp.netif);
      }
      memp_free(MEMP_TCPIP_MSG_INPKT, msg);//释放消息占用的内存
      break;
#endif /* LWIP_TCPIP_CORE_LOCKING_INPUT */

#if LWIP_NETIF_API
    case TCPIP_MSG_NETIFAPI:
      LWIP_DEBUGF(TCPIP_DEBUG, ("tcpip_thread: Netif API message %p\n", (void *)msg));
      msg->msg.netifapimsg->function(&(msg->msg.netifapimsg->msg));
      break;
#endif /* LWIP_NETIF_API */

#if LWIP_TCPIP_TIMEOUT
    case TCPIP_MSG_TIMEOUT://上层注册一个定时事件
      LWIP_DEBUGF(TCPIP_DEBUG, ("tcpip_thread: TIMEOUT %p\n", (void *)msg));
      sys_timeout(msg->msg.tmo.msecs, msg->msg.tmo.h, msg->msg.tmo.arg);
      memp_free(MEMP_TCPIP_MSG_API, msg);
      break;
    case TCPIP_MSG_UNTIMEOUT://上层删除一个当时函数
      LWIP_DEBUGF(TCPIP_DEBUG, ("tcpip_thread: UNTIMEOUT %p\n", (void *)msg));
      sys_untimeout(msg->msg.tmo.h, msg->msg.tmo.arg);
      memp_free(MEMP_TCPIP_MSG_API, msg);
      break;
#endif /* LWIP_TCPIP_TIMEOUT */

    case TCPIP_MSG_CALLBACK://上层通过回调方式执行一个函数
      LWIP_DEBUGF(TCPIP_DEBUG, ("tcpip_thread: CALLBACK %p\n", (void *)msg));
      msg->msg.cb.function(msg->msg.cb.ctx);
      memp_free(MEMP_TCPIP_MSG_API, msg);
      break;

    case TCPIP_MSG_CALLBACK_STATIC:
      LWIP_DEBUGF(TCPIP_DEBUG, ("tcpip_thread: CALLBACK_STATIC %p\n", (void *)msg));
      msg->msg.cb.function(msg->msg.cb.ctx);
      break;

    default:
      LWIP_DEBUGF(TCPIP_DEBUG, ("tcpip_thread: invalid message: %d\n", msg->type));
      LWIP_ASSERT("tcpip_thread: invalid message", 0);
      break;
    }
  }
}

/**向内核输入一个数据包消息
 * Pass a received packet to tcpip_thread for input processing
 *
 * @param p the received packet, p->payload pointing to the Ethernet header or
 *          to an IP header (if inp doesn't have NETIF_FLAG_ETHARP or
 *          NETIF_FLAG_ETHERNET flags)
 * @param inp the network interface on which the packet was received
 */
err_t
tcpip_input(struct pbuf *p, struct netif *inp)
{
#if LWIP_TCPIP_CORE_LOCKING_INPUT
  err_t ret;
  LWIP_DEBUGF(TCPIP_DEBUG, ("tcpip_input: PACKET %p/%p\n", (void *)p, (void *)inp));
  LOCK_TCPIP_CORE();
#if LWIP_ETHERNET
  if (inp->flags & (NETIF_FLAG_ETHARP | NETIF_FLAG_ETHERNET)) {
    ret = ethernet_input(p, inp);
  } else
#endif /* LWIP_ETHERNET */
  {
    ret = ip_input(p, inp);
  }
  UNLOCK_TCPIP_CORE();
  return ret;
#else /* LWIP_TCPIP_CORE_LOCKING_INPUT */
  struct tcpip_msg *msg;

  if (!sys_mbox_valid(&mbox)) {//判断系统邮箱是否有效
    return ERR_VAL;
  }
  msg = (struct tcpip_msg *)memp_malloc(MEMP_TCPIP_MSG_INPKT);//申请新的消息结构空间
  if (msg == NULL) {
    return ERR_MEM;
  }
//构造消息
  msg->type = TCPIP_MSG_INPKT;//类型为数据包消息
  msg->msg.inp.p = p;//初始化消息结构中msg共用体的inp字段
  msg->msg.inp.netif = inp;
  if (sys_mbox_trypost(&mbox, msg) != ERR_OK) {//向系统邮箱投递消息
    memp_free(MEMP_TCPIP_MSG_INPKT, msg);//失败则删除消息结构
    return ERR_MEM;
  }
  return ERR_OK;
#endif /* LWIP_TCPIP_CORE_LOCKING_INPUT */
}

/**
 * Call a specific function in the thread context of
 * tcpip_thread for easy access synchronization.
 * A function called in that way may access lwIP core code
 * without fearing concurrent access.
 *
 * @param f the function to call
 * @param ctx parameter passed to f
 * @param block 1 to block until the request is posted, 0 to non-blocking mode
 * @return ERR_OK if the function was called, another err_t if not
 */
err_t
tcpip_callback_with_block(tcpip_callback_fn function, void *ctx, u8_t block)
{
  struct tcpip_msg *msg;

  if (sys_mbox_valid(&mbox)) {
    msg = (struct tcpip_msg *)memp_malloc(MEMP_TCPIP_MSG_API);
    if (msg == NULL) {
      return ERR_MEM;
    }

    msg->type = TCPIP_MSG_CALLBACK;
    msg->msg.cb.function = function;
    msg->msg.cb.ctx = ctx;
    if (block) {
      sys_mbox_post(&mbox, msg);
    } else {
      if (sys_mbox_trypost(&mbox, msg) != ERR_OK) {
        memp_free(MEMP_TCPIP_MSG_API, msg);
        return ERR_MEM;
      }
    }
    return ERR_OK;
  }
  return ERR_VAL;
}

#if LWIP_TCPIP_TIMEOUT
/**
 * call sys_timeout in tcpip_thread
 *
 * @param msec time in milliseconds for timeout
 * @param h function to be called on timeout
 * @param arg argument to pass to timeout function h
 * @return ERR_MEM on memory error, ERR_OK otherwise
 */
err_t
tcpip_timeout(u32_t msecs, sys_timeout_handler h, void *arg)
{
  struct tcpip_msg *msg;

  if (sys_mbox_valid(&mbox)) {
    msg = (struct tcpip_msg *)memp_malloc(MEMP_TCPIP_MSG_API);
    if (msg == NULL) {
      return ERR_MEM;
    }

    msg->type = TCPIP_MSG_TIMEOUT;
    msg->msg.tmo.msecs = msecs;
    msg->msg.tmo.h = h;
    msg->msg.tmo.arg = arg;
    sys_mbox_post(&mbox, msg);
    return ERR_OK;
  }
  return ERR_VAL;
}

/**
 * call sys_untimeout in tcpip_thread
 *
 * @param msec time in milliseconds for timeout
 * @param h function to be called on timeout
 * @param arg argument to pass to timeout function h
 * @return ERR_MEM on memory error, ERR_OK otherwise
 */
err_t
tcpip_untimeout(sys_timeout_handler h, void *arg)
{
  struct tcpip_msg *msg;

  if (sys_mbox_valid(&mbox)) {
    msg = (struct tcpip_msg *)memp_malloc(MEMP_TCPIP_MSG_API);
    if (msg == NULL) {
      return ERR_MEM;
    }

    msg->type = TCPIP_MSG_UNTIMEOUT;
    msg->msg.tmo.h = h;
    msg->msg.tmo.arg = arg;
    sys_mbox_post(&mbox, msg);
    return ERR_OK;
  }
  return ERR_VAL;
}
#endif /* LWIP_TCPIP_TIMEOUT */

#if LWIP_NETCONN
/**向内核投递一个API消息，等待内核执行
 * Call the lower part of a netconn_* function
 * This function is then running in the thread context
 * of tcpip_thread and has exclusive access to lwIP core code.
 *
 * @param apimsg a struct containing the function to call and its parameters
 * @return ERR_OK if the function was called, another err_t if not
 */
err_t
tcpip_apimsg(struct api_msg *apimsg)
{
  struct tcpip_msg msg;
#ifdef LWIP_DEBUG
  /* catch functions that don't set err */
  apimsg->msg.err = ERR_VAL;
#endif
  
  if (sys_mbox_valid(&mbox)) {//内核邮箱有效,构造消息
    msg.type = TCPIP_MSG_API;//消息类型为：API函数调用，在tcpip_thread中循环识别
    msg.msg.apimsg = apimsg;//记录api消息
    sys_mbox_post(&mbox, &msg);//投递消息
    sys_arch_sem_wait(&apimsg->msg.conn->op_completed, 0);//等待信号量op_completed
    return apimsg->msg.err;
  }
  return ERR_VAL;
}

#if LWIP_TCPIP_CORE_LOCKING
/**
 * Call the lower part of a netconn_* function
 * This function has exclusive access to lwIP core code by locking it
 * before the function is called.
 *
 * @param apimsg a struct containing the function to call and its parameters
 * @return ERR_OK (only for compatibility fo tcpip_apimsg())
 */
err_t
tcpip_apimsg_lock(struct api_msg *apimsg)
{
#ifdef LWIP_DEBUG
  /* catch functions that don't set err */
  apimsg->msg.err = ERR_VAL;
#endif

  LOCK_TCPIP_CORE();
  apimsg->function(&(apimsg->msg));
  UNLOCK_TCPIP_CORE();
  return apimsg->msg.err;

}
#endif /* LWIP_TCPIP_CORE_LOCKING */
#endif /* LWIP_NETCONN */

#if LWIP_NETIF_API
#if !LWIP_TCPIP_CORE_LOCKING
/**
 * Much like tcpip_apimsg, but calls the lower part of a netifapi_*
 * function.
 *
 * @param netifapimsg a struct containing the function to call and its parameters
 * @return error code given back by the function that was called
 */
err_t
tcpip_netifapi(struct netifapi_msg* netifapimsg)
{
  struct tcpip_msg msg;
  
  if (sys_mbox_valid(&mbox)) {
    err_t err = sys_sem_new(&netifapimsg->msg.sem, 0);
    if (err != ERR_OK) {
      netifapimsg->msg.err = err;
      return err;
    }
    
    msg.type = TCPIP_MSG_NETIFAPI;
    msg.msg.netifapimsg = netifapimsg;
    sys_mbox_post(&mbox, &msg);
    sys_sem_wait(&netifapimsg->msg.sem);
    sys_sem_free(&netifapimsg->msg.sem);
    return netifapimsg->msg.err;
  }
  return ERR_VAL;
}
#else /* !LWIP_TCPIP_CORE_LOCKING */
/**
 * Call the lower part of a netifapi_* function
 * This function has exclusive access to lwIP core code by locking it
 * before the function is called.
 *
 * @param netifapimsg a struct containing the function to call and its parameters
 * @return ERR_OK (only for compatibility fo tcpip_netifapi())
 */
err_t
tcpip_netifapi_lock(struct netifapi_msg* netifapimsg)
{
  LOCK_TCPIP_CORE();  
  netifapimsg->function(&(netifapimsg->msg));
  UNLOCK_TCPIP_CORE();
  return netifapimsg->msg.err;
}
#endif /* !LWIP_TCPIP_CORE_LOCKING */
#endif /* LWIP_NETIF_API */

/**
 * Allocate a structure for a static callback message and initialize it.
 * This is intended to be used to send "static" messages from interrupt context.
 *
 * @param function the function to call
 * @param ctx parameter passed to function
 * @return a struct pointer to pass to tcpip_trycallback().
 */
struct tcpip_callback_msg* tcpip_callbackmsg_new(tcpip_callback_fn function, void *ctx)
{
  struct tcpip_msg *msg = (struct tcpip_msg *)memp_malloc(MEMP_TCPIP_MSG_API);
  if (msg == NULL) {
    return NULL;
  }
  msg->type = TCPIP_MSG_CALLBACK_STATIC;
  msg->msg.cb.function = function;
  msg->msg.cb.ctx = ctx;
  return (struct tcpip_callback_msg*)msg;
}

/**
 * Free a callback message allocated by tcpip_callbackmsg_new().
 *
 * @param msg the message to free
 */
void tcpip_callbackmsg_delete(struct tcpip_callback_msg* msg)
{
  memp_free(MEMP_TCPIP_MSG_API, msg);
}

/**
 * Try to post a callback-message to the tcpip_thread mbox
 * This is intended to be used to send "static" messages from interrupt context.
 *
 * @param msg pointer to the message to post
 * @return sys_mbox_trypost() return code
 */
err_t
tcpip_trycallback(struct tcpip_callback_msg* msg)
{
  if (!sys_mbox_valid(&mbox)) {
    return ERR_VAL;
  }
  return sys_mbox_trypost(&mbox, msg);
}

/**
 * Initialize this module:
 * - initialize all sub modules
 * - start the tcpip_thread
 *
 * @param initfunc a function to call when tcpip_thread is running and finished initializing
 * @param arg argument to pass to initfunc
 */
void
tcpip_init(tcpip_init_done_fn initfunc, void *arg)
{
  lwip_init();//内核初始化

  tcpip_init_done = initfunc;//注册用户自定义的初始化函数
  tcpip_init_done_arg = arg;//消息参数
  if(sys_mbox_new(&mbox, TCPIP_MBOX_SIZE) != ERR_OK) {//创建内核邮箱
    LWIP_ASSERT("failed to create tcpip_thread mbox", 0);
  }
#if LWIP_TCPIP_CORE_LOCKING
  if(sys_mutex_new(&lock_tcpip_core) != ERR_OK) {
    LWIP_ASSERT("failed to create lock_tcpip_core", 0);
  }
#endif /* LWIP_TCPIP_CORE_LOCKING */

  sys_thread_new(TCPIP_THREAD_NAME, tcpip_thread, NULL, TCPIP_THREAD_STACKSIZE, TCPIP_THREAD_PRIO);//创建系统进程
}

/**
 * Simple callback function used with tcpip_callback to free a pbuf
 * (pbuf_free has a wrong signature for tcpip_callback)
 *
 * @param p The pbuf (chain) to be dereferenced.
 */
static void
pbuf_free_int(void *p)
{
  struct pbuf *q = (struct pbuf *)p;
  pbuf_free(q);
}

/**
 * A simple wrapper function that allows you to free a pbuf from interrupt context.
 *
 * @param p The pbuf (chain) to be dereferenced.
 * @return ERR_OK if callback could be enqueued, an err_t if not
 */
err_t
pbuf_free_callback(struct pbuf *p)
{
  return tcpip_callback_with_block(pbuf_free_int, p, 0);
}

/**
 * A simple wrapper function that allows you to free heap memory from
 * interrupt context.
 *
 * @param m the heap memory to free
 * @return ERR_OK if callback could be enqueued, an err_t if not
 */
err_t
mem_free_callback(void *m)
{
  return tcpip_callback_with_block(mem_free, m, 0);
}

#endif /* !NO_SYS */
