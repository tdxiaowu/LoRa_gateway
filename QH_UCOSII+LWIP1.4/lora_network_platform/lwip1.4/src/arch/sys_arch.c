/*
 * Copyright (c) 2001, Swedish Institute of Computer Science.
 * All rights reserved. 
 *
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions 
 * are met: 
 * 1. Redistributions of source code must retain the above copyright 
 *    notice, this list of conditions and the following disclaimer. 
 * 2. Redistributions in binary form must reproduce the above copyright 
 *    notice, this list of conditions and the following disclaimer in the 
 *    documentation and/or other materials provided with the distribution. 
 * 3. Neither the name of the Institute nor the names of its contributors 
 *    may be used to endorse or promote products derived from this software 
 *    without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND 
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE 
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL 
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS 
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) 
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT 
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY 
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF 
 * SUCH DAMAGE. 
 *
 * This file is part of the lwIP TCP/IP stack.
 * 
 * Author: Adam Dunkels <adam@sics.se>
 *
 * $Id: sys_arch.c,v 1.1.1.1 2003/05/17 05:06:56 chenyu Exp $
 */

#include "../include/lwip/debug.h"
#include "../include/lwip/def.h"
#include "../include/lwip/sys.h"
#include "../include/lwip/mem.h"
#include "lwip/err.h"
#include "sys_arch.h" 

#include <string.h>
volatile u32_t system_tick_num = 0;//无操作系统时,lwip协议栈更新时钟
/*定义：提取系统现在时钟，被sys_check_timeouts调用，更新系统时间
 * 返回：系统现在时钟
 * 系统现在时间，该函数被许多需要定时和提取当前时间的函数调用，用于获取现在的系统时间
 */
u32_t sys_now(void)
{
    return system_tick_num;
}
//#define first
#ifdef first
#if !NO_SYS

//ucosii的内存管理结构，我们将所有邮箱空间通过内存管理结构来管理

/*
static OS_MEM *MboxMem;
static char MboxMemoryArea[TOTAL_MBOX_NUM * sizeof(struct LWIP_MBOX_STRUCT)];
const u32_t NullMessage;//解决空指针投递的问题
*/

//定义系统使用的超时链表首指针结构
//struct sys_timeouts global_timeouts;
//与系统任务新建函数相关的变量定义

#define LWIP_MAX_TASKS 4 	//允许内核最多创建的任务个数
#define LWIP_STK_SIZE  512	//每个任务的堆栈空间
OS_STK  LWIP_STK_AREA[LWIP_MAX_TASKS][LWIP_STK_SIZE];


void sys_init()
{
  //currently do nothing
  printf("[Sys_arch] init ok");
}

err_t sys_sem_new(sys_sem_t *sem, u8_t count)
{
  OS_EVENT *new_sem = NULL;

  LWIP_ASSERT("[Sys_arch]sem != NULL", sem != NULL);

  new_sem = OSSemCreate((u16_t)count);
  LWIP_ASSERT("[Sys_arch]Error creating sem", new_sem != NULL);
  if(new_sem != NULL) {
    *sem = (void *)new_sem;
    return ERR_OK;
  }

  *sem = SYS_SEM_NULL;
  return ERR_MEM;
}

void sys_sem_free(sys_sem_t *sem)
{
  u8_t Err;
  // parameter check
  LWIP_ASSERT("sem != NULL", sem != NULL);

  OSSemDel(*sem, OS_DEL_ALWAYS, &Err);

  if(Err != OS_ERR_NONE)
  {
    //add error log here
    printf("[Sys_arch]free sem fail\n");
  }

  *sem = NULL;
}

u32_t sys_arch_sem_wait(sys_sem_t *sem, u32_t timeout)
{
  u8_t Err;
  u32_t wait_ticks;
  u32_t start, end;
  LWIP_ASSERT("sem != NULL", sem != NULL);

  if (OSSemAccept(*sem))		  // 如果已经收到, 则返回0
  {
	  //Printf("debug:sem accept ok\n");
	  return 0;
  }

  wait_ticks = 0;
  if(timeout!=0){
	 wait_ticks = (timeout * OS_TICKS_PER_SEC)/1000;
	 if(wait_ticks < 1)
		wait_ticks = 1;
	 else if(wait_ticks > 65535)
			wait_ticks = 65535;
  }

  start = sys_now();
  OSSemPend(*sem, (u16_t)wait_ticks, &Err);
  end = sys_now();

  if (Err == OS_ERR_NONE)
		return (u32_t)(end - start);		//将等待时间设置为timeout/2
  else
		return SYS_ARCH_TIMEOUT;

}

void sys_sem_signal(sys_sem_t *sem)
{
  u8_t Err;
  LWIP_ASSERT("sem != NULL", sem != NULL);

  Err = OSSemPost(*sem);
  if(Err != OS_ERR_NONE)
  {
        //add error log here
        printf("[Sys_arch]:signal sem fail\n");
  }

  LWIP_ASSERT("Error releasing semaphore", Err == OS_ERR_NONE);
}

err_t sys_mbox_new(sys_mbox_t *mbox, int size)
{
  err_t err;
  LWIP_ASSERT("mbox != NULL", mbox != NULL);
  LWIP_UNUSED_ARG(size);

  err = sys_sem_new(&(mbox->sem), 0);
  LWIP_ASSERT("Error creating semaphore", err == ERR_OK);
  if(err != ERR_OK) {
    return ERR_MEM;
  }
  err = sys_mutex_new(&(mbox->mutex));
  LWIP_ASSERT("Error creating mutex", err == ERR_OK);
  if(err != ERR_OK) {
  	sys_sem_free(&(mbox->sem));
    return ERR_MEM;
  }

  memset(&mbox->q_mem, 0, sizeof(void *)*MAX_QUEUE_ENTRIES);
  mbox->head = 0;
  mbox->tail = 0;
  mbox->msg_num = 0;

  return ERR_OK;
}

void sys_mbox_free(sys_mbox_t *mbox)
{
  /* parameter check */
  u8_t Err;
  LWIP_ASSERT("mbox != NULL", mbox != NULL);

  sys_sem_free(&(mbox->sem));
  sys_mutex_free(&(mbox->mutex));

  mbox->sem = NULL;
  mbox->mutex = NULL;
}

void sys_mbox_post(sys_mbox_t *q, void *msg)
{
  u8_t Err;
  //SYS_ARCH_DECL_PROTECT(lev);

  /* parameter check */
  LWIP_ASSERT("q != SYS_MBOX_NULL", q != SYS_MBOX_NULL);
  LWIP_ASSERT("q->sem != NULL", q->sem != NULL);

  //queue is full, we wait for some time
  while(q->msg_num >= MAX_QUEUE_ENTRIES)
  {
    //OSTimeDly(1);//sys_msleep(20);
    sys_msleep(1);
  }

  //SYS_ARCH_PROTECT(lev);
  sys_mutex_lock(&(q->mutex));
  if(q->msg_num >= MAX_QUEUE_ENTRIES)
  {
    LWIP_ASSERT("mbox post error, we can not handle it now, Just drop msg!", 0);
	//SYS_ARCH_UNPROTECT(lev);
	sys_mutex_unlock(&(q->mutex));
	return;
  }
  q->q_mem[q->head] = msg;
  (q->head)++;
  if (q->head >= MAX_QUEUE_ENTRIES) {
    q->head = 0;
  }

  q->msg_num++;
  if(q->msg_num == MAX_QUEUE_ENTRIES)
  {
    printf("mbox post, box full\n");
  }

  //Err = OSSemPost(q->sem);
  sys_sem_signal(&(q->sem));
  //if(Err != OS_ERR_NONE)
  //{
    //add error log here
  //  Printf("[Sys_arch]:mbox post sem fail\n");
  //}

  //SYS_ARCH_UNPROTECT(lev);
  sys_mutex_unlock(&(q->mutex));
}

err_t sys_mbox_trypost(sys_mbox_t *q, void *msg)
{
  u8_t Err;
  //SYS_ARCH_DECL_PROTECT(lev);

  /* parameter check */
  LWIP_ASSERT("q != SYS_MBOX_NULL", q != SYS_MBOX_NULL);
  LWIP_ASSERT("q->sem != NULL", q->sem != NULL);

  //SYS_ARCH_PROTECT(lev);
  sys_mutex_lock(&(q->mutex));

  if (q->msg_num >= MAX_QUEUE_ENTRIES) {
    //SYS_ARCH_UNPROTECT(lev);
    sys_mutex_unlock(&(q->mutex));
	printf("[Sys_arch]:mbox try post mbox full\n");
    return ERR_MEM;
  }

  q->q_mem[q->head] = msg;
  (q->head)++;
  if (q->head >= MAX_QUEUE_ENTRIES) {
    q->head = 0;
  }

  q->msg_num++;
  if(q->msg_num == MAX_QUEUE_ENTRIES)
  {
    printf("mbox try post, box full\n");
  }

  //Err = OSSemPost(q->sem);
  sys_sem_signal(&(q->sem));
  //if(Err != OS_ERR_NONE)
  //{
    //add error log here
  //  Printf("[Sys_arch]:mbox try post sem fail\n");
  //}
  //SYS_ARCH_UNPROTECT(lev);
  sys_mutex_unlock(&(q->mutex));
  return ERR_OK;
}

u32_t sys_arch_mbox_fetch(sys_mbox_t *q, void **msg, u32_t timeout)
{
  u8_t Err;
  u32_t wait_ticks;
  u32_t start, end;
  u32_t tmp_num;
  //SYS_ARCH_DECL_PROTECT(lev);

  // parameter check
  LWIP_ASSERT("q != SYS_MBOX_NULL", q != SYS_MBOX_NULL);
  LWIP_ASSERT("q->sem != NULL", q->sem != NULL);

  //while(q->msg_num == 0)
  //{
  //  OSTimeDly(1);//sys_msleep(20);
  //}

  wait_ticks = 0;
  if(timeout!=0){
	 wait_ticks = (timeout * OS_TICKS_PER_SEC)/1000;
	 if(wait_ticks < 1)
		wait_ticks = 1;
	 else if(wait_ticks > 65535)
			wait_ticks = 65535;
  }

  //start = sys_now();
  //OSSemPend(q->sem, (u16_t)wait_ticks, &Err);
  start = sys_arch_sem_wait(&(q->sem), timeout);
  //end = sys_now();

  if (start != SYS_ARCH_TIMEOUT)
  {
    //SYS_ARCH_PROTECT(lev);
    sys_mutex_lock(&(q->mutex));

	if(q->head == q->tail)
	{
        printf("mbox fetch queue abnormal [%u]\n", q->msg_num);
		if(msg != NULL) {
			*msg  = NULL;
	    }
		//SYS_ARCH_UNPROTECT(lev);
		sys_mutex_unlock(&(q->mutex));
		return SYS_ARCH_TIMEOUT;
	}

    if(msg != NULL) {
      *msg  = q->q_mem[q->tail];
    }

    (q->tail)++;
    if (q->tail >= MAX_QUEUE_ENTRIES) {
      q->tail = 0;
    }

	if(q->msg_num > 0)
	{
      q->msg_num--;
	}
	else
	{
      printf("mbox fetch queue error [%u]\n", q->msg_num);
	}

	tmp_num = (q->head >= q->tail)?(q->head - q->tail):(MAX_QUEUE_ENTRIES + q->head - q->tail);

	if(tmp_num != q->msg_num)
	{
        printf("mbox fetch error, umatch [%u] with tmp [%u]\n", q->msg_num, tmp_num);
	}

	//SYS_ARCH_UNPROTECT(lev);
	sys_mutex_unlock(&(q->mutex));
	//Printf("mbox fetch ok, match [%u] with tmp [%u] \n", q->msg_num, tmp_num);
	//return (u32_t)(end - start);		//将等待时间设置为timeout/2;
	return start;
  }
  else
  {
    //Printf("mbox fetch time out error");
    if(msg != NULL) {
      *msg  = NULL;
    }

	return SYS_ARCH_TIMEOUT;
  }
}

u32_t sys_arch_mbox_tryfetch(sys_mbox_t *q, void **msg)
{
  u32_t tmp_num;
  //SYS_ARCH_DECL_PROTECT(lev);
  u32_t start;
  /* parameter check */
  LWIP_ASSERT("q != SYS_MBOX_NULL", q != SYS_MBOX_NULL);
  LWIP_ASSERT("q->sem != NULL", q->sem != NULL);

  if(q->msg_num == 0)
  	return SYS_MBOX_EMPTY;

  start = sys_arch_sem_wait(&(q->sem), 1);

  if (start != SYS_ARCH_TIMEOUT) {
    //SYS_ARCH_PROTECT(lev);
    sys_mutex_lock(&(q->mutex));
	if(q->head == q->tail)
	{
        printf("mbox tryfetch queue abnormal [%u]\n", q->msg_num);
		if(msg != NULL) {
			*msg  = NULL;
	    }
		//SYS_ARCH_UNPROTECT(lev);
		sys_mutex_unlock(&(q->mutex));
		return SYS_MBOX_EMPTY;
	}

    if(msg != NULL) {
      *msg  = q->q_mem[q->tail];
    }

    (q->tail)++;
    if (q->tail >= MAX_QUEUE_ENTRIES) {
      q->tail = 0;
    }

    if(q->msg_num > 0)
	{
      q->msg_num--;
	}

	tmp_num = (q->head >= q->tail)?(q->head - q->tail):(MAX_QUEUE_ENTRIES + q->head - q->tail);


	if(tmp_num != q->msg_num)
	{
        printf("mbox try fetch error, umatch [%u] with tmp [%u]\n", q->msg_num, tmp_num);
	}

    //SYS_ARCH_UNPROTECT(lev);
    sys_mutex_unlock(&(q->mutex));
    return 0;
  }
  else
  {
    printf("mbox try fetch uknow error\n");
    if(msg != NULL) {
      *msg  = NULL;
    }

    return SYS_MBOX_EMPTY;
  }
}

//函数功能：新建一个进程，在整个系统中只会被调用一次
//sys_thread_t sys_thread_new(char *name, void (* thread)(void *arg), void *arg, int stacksize, int prio);
//prio 1~10, is kept for network
//TCPIP_THREAD_PRIO    1   -> lwip thead prio
sys_thread_t sys_thread_new(const char *name, lwip_thread_fn thread, void *arg, int stacksize, int prio)
{
  static u32_t TaskCreateFlag=0;
  u8_t i=0;
//  name=name;
//  stacksize=stacksize;

  while((TaskCreateFlag>>i)&0x01){
    if(i<LWIP_MAX_TASKS&&i<32)
          i++;
    else return 0;
  }
  if(OSTaskCreate(thread, (void *)arg, &LWIP_STK_AREA[i][LWIP_STK_SIZE-1],prio)==OS_ERR_NONE){
       TaskCreateFlag |=(0x01<<i);

  };

  return prio;
}

#endif


#else
#if !NO_SYS
//与系统任务创建函数相关的全局变量，包括了允许的最大任务数和相关堆栈空间
#define LWIP_MAX_TASKS 2		//允许协议栈创建的最大任务数
#define LWIP_STK_SIZE  800		//每个任务的堆栈空间大小

/*********************/

//定义任务堆栈区域，为任务提供堆栈空间
OS_STK LWIP_TASK_STK[LWIP_MAX_TASKS][LWIP_STK_SIZE];

/*
 * sys_init
 * 完成用户所需的操作系统模拟层初始化工作
 */
void sys_init(void)
{
	printf("sys_arch init OK.\n");
}

/****************信号量函数******************************/
/*
 * sys_sem_new
 * 创建一个信号量，信号初始值为count;创建成功则返回ERR_OK，否则返回其他值
 * sem指向成功创建的信号量
 */
err_t sys_sem_new(sys_sem_t *sem, u8_t count)
{
	OS_EVENT *new_sem = NULL;   //定义一个信号量变量

	LWIP_ASSERT("[Sys_arch]sem != NULL", sem != NULL);

	new_sem = OSSemCreate((INT16U)count);			//利用操作系统创建一个信号量
	LWIP_ASSERT("[Sys_arch]Error creating sem", new_sem != NULL);
	if(new_sem != NULL)								//若创建成功，则记录信号量
	{
		*sem = (void *)new_sem;//?为什么用void
		return ERR_OK;
	}

	*sem = SYS_SEM_NULL;		//创建失败，返回内存错误
	return ERR_MEM;
}

/*
 * sys_sem_signal
 * 释放一个信号量
 * 传入参数是信号量sem指针的地址
 */
void sys_sem_signal(sys_sem_t *sem)
{
	INT8U err;		//定义错误接收变量
	LWIP_ASSERT("sem != NULL", sem != NULL);

	err = OSSemPost (*sem);//释放一个信号量
	if(err != OS_ERR_NONE){
		printf("sys_arch:signal sem fail.\n");
	}

	LWIP_ASSERT("Error releasing semaphore", err == OS_ERR_NONE);
}
/*
 * 等待信号量，如果timeout = 0 永久等待，即函数一直阻塞在信号量上．
 * 如果timeout不为０，表示函数会阻塞的最大毫秒数，返回值为等待信号量的等待时间，若timeout内没有等到信号量返回值SYS_ARCH_TIMEOUT
 * 若信号量在调用函数时已可用，则函数不会发生任何阻塞，返回值可以是０．
 */
u32_t sys_arch_sem_wait(sys_sem_t *sem, u32_t timeout)
{
	INT8U err;
	u32_t wait_ticks;
	u32_t start,end;

	LWIP_ASSERT("sem != NULL", sem != NULL);

	//如果有信号量可用，直接返回０
	if(OSSemAccept (*sem))//请求一个信号量．无阻塞
	{
		return 0;
	}

	//如果没有可用信号量，等待
	wait_ticks = 0;
	if(timeout != 0)
	{
		wait_ticks = (timeout * OS_TICKS_PER_SEC) / 1000;//将等待的毫秒数转换为操作系统中对应的滴答数

		if(wait_ticks < 1)
			wait_ticks = 1;
		else if(wait_ticks > 65535)
			wait_ticks = 65535;
	}

	start = sys_now();		//获取等待开始时间
	OSSemPend (*sem,         //被请求信号量的指针
	           wait_ticks,   //等待时限
	           &err);        //错误信息
	end = sys_now();		//获取等待结束时间

	//查看错误类型
	if(err == OS_ERR_NONE)
		return (u32_t)(end - start);
	else
		return SYS_ARCH_TIMEOUT;
}

/*
 * sys_sem_free
 * 删除sem指向的信号量
 */
void sys_sem_free(sys_sem_t *sem)
{
	INT8U err;
	LWIP_ASSERT("sem != NULL", sem != NULL);

	//直接调用操作系统信号量删除函数
	OSSemDel (*sem,
			  OS_DEL_ALWAYS,
	          &err);
	if(err != OS_ERR_NONE)
	{
		printf("sys_arch:free sem fail.\n");
	}

	*sem = NULL;
}



/****************邮箱函数******************************/

/*
 * sys_mbox_new
 * 创建一个邮箱,初始化一个邮箱
 * 邮箱能容纳的消息数为size,本实现中，忽略了参数size，每个邮箱默认的大小为MAX_QUEUE_ENTRIES
 * mbox为调用者已分配好的指向邮箱的指针
 * 创建成功返回ERR_OK
 */
err_t sys_mbox_new(sys_mbox_t *mbox, int size)
{
	err_t err;
	LWIP_ASSERT("mbox != NULL", mbox != NULL);
	LWIP_UNUSED_ARG(size);	//	忽略邮箱大小，使用默认值
	err = sys_sem_new(&(mbox->sem), 0);//首先创建邮箱生的信号量，用于同步
	if(err !=ERR_OK)//创建失败，则返回内存错误
	{
		return ERR_MEM;
	}
	LWIP_ASSERT("Error creating semaphore",err == ERR_OK);

	err = sys_mutex_new(&(mbox->mutex));//创建邮箱上的互斥信号量
	if(err != ERR_OK)
	{
		sys_sem_free(&(mbox->sem));//清空已申请的资源
		return ERR_MEM;//返回内存错误
	}
	LWIP_ASSERT("Error creating mutex",err == ERR_OK);

	//信号量和互斥量创建成功，初始化环形缓冲区
	memset(&(mbox->q_mem),0,sizeof(void*) * MAX_QUEUE_ENTRIES);
	mbox->head = 0;
	mbox->tail = 0;
	mbox->msg_num = 0;

	return ERR_OK;
}

/*
 * sys_mbox_free
 * 删除邮箱，如果删除时邮箱中还包含消息，应避免这种情况发生
 */
void sys_mbox_free(sys_mbox_t *mbox)
{
	LWIP_ASSERT("mbox != NULL", mbox != NULL);

	sys_sem_free(&(mbox->sem));//删除信号量
	sys_sem_free(&(mbox->mutex));//删除互斥量

	mbox->sem = NULL;
	mbox->mutex = NULL;

}


/*
 *  sys_mbox_post（带阻塞）
 *  向邮箱发送一条消息，如果发送队列满，则这个函数阻塞，直到发送成功
 *  向缓冲队列内插入一条数据，更新消息计数，调整写入指针
 */
void sys_mbox_post(sys_mbox_t *mbox, void *msg)
{
	LWIP_ASSERT("mbox != SYS_MBOX_NULL",mbox != SYS_MBOX_NULL);
	LWIP_ASSERT("mbox->sem != NULL",mbox->sem != NULL);

	while(mbox->msg_num >= MAX_QUEUE_ENTRIES)
	{
		sys_msleep(1);
	}

	sys_mutex_lock(&(mbox->mutex));//获得操作缓冲区的互斥锁
	if(mbox->msg_num >= MAX_QUEUE_ENTRIES)//在等待锁期间，缓冲区再次被填满
	{
		LWIP_ASSERT("mbox post error, we can not handle it now, Just drop msg!", 0);

		sys_mutex_unlock(&(mbox->mutex));//无法处理这种竞争关系
		return;
	}

	//如果可以写入消息
	mbox->q_mem[mbox->head] = msg;//写入消息
	(mbox->head)++;//调整写入指针
	if(mbox->head >= MAX_QUEUE_ENTRIES)//若指针已经到达缓冲区末尾
	{
		mbox->head = 0;//调整指针到缓冲区首部
	}

	(mbox->msg_num)++;//增加消息计数
	if(mbox->msg_num == MAX_QUEUE_ENTRIES)
	{
		printf("mbox post, box full\n");
	}

	sys_sem_signal(&(mbox->sem));//消息正常投递，释放一个信号量
	sys_mutex_unlock(&(mbox->mutex));//释放缓冲区互斥量
}
/*
 * sys_mbox_trypost()
 * 尝试向邮箱发送消息，成功返回ERR_OK,若邮箱已满，返回ERR_MEM
 */
err_t sys_mbox_trypost(sys_mbox_t *mbox, void *msg)
{
	  /* parameter check */
	LWIP_ASSERT("mbox != SYS_MBOX_NULL", mbox != SYS_MBOX_NULL);
	LWIP_ASSERT("mbox->sem != NULL", mbox->sem != NULL);

	sys_mutex_lock(&(mbox->mutex));//获得操作缓冲区的互斥锁

	if (mbox->msg_num >= MAX_QUEUE_ENTRIES)//缓冲区满，写失败
	{
		sys_mutex_unlock(&(mbox->mutex));
		printf("[Sys_arch]:mbox try post mbox full\n");
		return ERR_MEM;
	}

	//缓冲区未满，写入消息
	mbox->q_mem[mbox->head] = msg;//写入消息
	(mbox->head)++;//调整写入指针
	if(mbox->head >= MAX_QUEUE_ENTRIES)//若指针已经到达缓冲区末尾
	{
	  mbox->head = 0;//调整指针到缓冲区首部
	}

	(mbox->msg_num)++;//增加消息计数
	if(mbox->msg_num == MAX_QUEUE_ENTRIES)
	{
	  printf("mbox post, box full\n");
	}

	sys_sem_signal(&(mbox->sem));//消息正常投递，释放一个信号量
	sys_mutex_unlock(&(mbox->mutex));//释放缓冲区互斥量

	return ERR_OK;
}

/*
 * sys_arch_mbox_fetch
 * 从邮箱中获取消息
 */
/** Wait for a new message to arrive in the mbox
 * @param mbox mbox to get a message from
 * @param msg pointer where the message is stored
 * @param timeout maximum time (in milliseconds) to wait for a message
 * @return time (in milliseconds) waited for a message, may be 0 if not waited
           or SYS_ARCH_TIMEOUT on timeout
 *         The returned time has to be accurate to prevent timer jitter! */
u32_t sys_arch_mbox_fetch(sys_mbox_t *mbox, void **msg, u32_t timeout)
{
	u32_t wait_ticks;//阻塞时间

	wait_ticks = sys_arch_sem_wait(&(mbox->sem),timeout);//阻塞信号量，直至邮箱中有数据或超时

	if(wait_ticks != SYS_ARCH_TIMEOUT)//访问到信号量
	{
		sys_mutex_lock(&(mbox->mutex));//获得缓冲区访问锁

		if(msg != NULL)
		{
			*msg = mbox->q_mem[mbox->tail];//读取数据
		}

		(mbox->tail)++;//调整读数据指针
		if(mbox->tail >= MAX_QUEUE_ENTRIES)
		{
			mbox->tail = 0;
		}

		if(mbox->msg_num > 0)//减少邮箱中的消息数
		{
			mbox->msg_num--;
		}
		else
		{
			printf("mbox fetch queue error [%u]\n", mbox->msg_num);
		}

		sys_mutex_unlock(&(mbox->mutex));//释放缓冲区互斥量

		return wait_ticks;//返回阻塞时间
	}
	else/*wait_ticks != SYS_ARCH_TIMEOUT*///获取信号量超时
	{
		if(msg != NULL)
		{
			*msg = NULL;
		}

		return SYS_ARCH_TIMEOUT;//返回超时错误
	}
}

/*
 * sys_arch_mbox_tryfetch
 * 尝试从一个邮箱中读取消息，
 * 该函数不会阻塞进程，当邮箱中有数据时，且读取成，返回０，否则立即返回SYS_MBOX_EMPTY
 */
/** Wait for a new message to arrive in the mbox
 * @param mbox mbox to get a message from
 * @param msg pointer where the message is stored
 * @param timeout maximum time (in milliseconds) to wait for a message
 * @return 0 (milliseconds) if a message has been received
 *         or SYS_MBOX_EMPTY if the mailbox is empty */
u32_t sys_arch_mbox_tryfetch(sys_mbox_t *mbox, void **msg)
{
	u32_t wait_ticks;
	/* parameter check */
	LWIP_ASSERT("mbox != SYS_MBOX_NULL", mbox != SYS_MBOX_NULL);
	LWIP_ASSERT("mbox->sem != NULL", mbox->sem != NULL);
	if(mbox->msg_num == 0)
	{
		return SYS_MBOX_EMPTY;
	}

	wait_ticks = sys_arch_sem_wait(&(mbox->sem),1);

	if(wait_ticks != SYS_ARCH_TIMEOUT)//成功获得信号量
	{
		sys_mutex_lock(&(mbox->mutex));//获得缓冲区访问锁

		if(msg != NULL)
		{
			*msg = mbox->q_mem[mbox->tail];//读取消息
		}

		(mbox->tail)++;//调整读数据指针
		if(mbox->tail >= MAX_QUEUE_ENTRIES)
		{
			mbox->tail = 0;
		}

		if(mbox->msg_num > 0)
		{
			mbox->msg_num--;
		}

		sys_mutex_unlock(&(mbox->mutex));

		return 0;
	}
	else/*wait_ticks != SYS_ARCH_TIMEOUT*/
	{
		if(msg != NULL)
		{
			*msg = NULL;
		}

		return SYS_ARCH_TIMEOUT;//返回超时错误
	}

}

/*
 * sys_thread_new
 *
 */
/*---------创建新进程-----------
 * 在操作环境模拟层的支持下，协议栈初始化*/
//函数功能：新建一个进程，在整个系统中只会被调用一次
sys_thread_t sys_thread_new(const char *name, lwip_thread_fn thread, void *arg, int stacksize, int prio)
{
  static u32_t TaskCreateFlag=0;//静态变量，标识堆栈空间的占用情况
  u8_t i=0;
  LWIP_UNUSED_ARG(name);	//	忽略的参数
  LWIP_UNUSED_ARG(stacksize);	//	忽略的参数

  while((TaskCreateFlag>>i)&0x01)//为任务寻找堆栈空间
  {
    if(i<LWIP_MAX_TASKS&&i<32)
          i++;
    else return 0;           //堆栈空间全部被占用，返回０
  }
  //调用操作系统函数创建进程，ｉ记录了可用的堆栈空间索引
#if (OS_TASK_STAT_EN == 0)
  if(OSTaskCreate(thread,//函数指针
		  	  	  (void *)arg,//参数
				  &LWIP_TASK_STK[i][LWIP_STK_SIZE-1],//栈顶指针
				  prio) == OS_ERR_NONE)//优先级
  {
       TaskCreateFlag |=(0x01<<i);
  };
#else
       if( OSTaskCreateExt(thread,
        		   (void *)arg,
				   &LWIP_STK_AREA[i][LWIP_STK_SIZE-1],
				   prio,
				   prio,
				   &LWIP_STK_AREA[i][0],
				   stacksize,(void *)0,
				   OS_TASK_OPT_STK_CHK | OS_TASK_OPT_STK_CLR) == OS_ERR_NONE)
       {
    	   TaskCreateFlag |=(0x01<<i);//设置堆栈使用标志
       }
#endif
  return prio;//返回进程优先级号作为进程编号
}
#endif//!NO_SYS
#endif
