#ifndef __SYS_ARCH_H__
#define __SYS_ARCH_H__
/*
 * 在本文件的实现中，为了减少lwip和OS间的耦合性，只使用OS的信号量机制来完成模拟层中的邮箱和信号量函数的设计*/
#include <ucos_ii.h>
#if !NO_SYS
//定义信号量相关的类型和宏

typedef OS_EVENT* sys_sem_t;    //定义信号量类型为操作系统上的一个事件指针，该类型在sys_arch.txt中有说明

#define SYS_SEM_NULL  NULL      //空信号量
#define sys_sem_valid(sem)  ((*sem) != NULL)  //判断sem指向的信号量是否有效
#define sys_sem_set_invalid(sem)  ((*sem) = NULL)  //将sem指向的信号量置为无效


//定义互斥量相关的类型和宏
#define LWIP_COMPAT_MUTEX  1   //启动内核基于信号量函数来实现互斥量函数


//定义邮箱相关的类型和宏，邮箱采用环形缓冲机制来实现
#define MAX_QUEUE_ENTRIES  100    //定义邮箱可缓冲的最大消息数量
//定义邮箱结构体，利用信号量实现
struct lwip_mbox{
	sys_sem_t sem;                 //信号量指针，用于邮箱内消息的同步访问
	sys_sem_t mutex;               //互斥量指针，用于邮箱内缓存区的互斥访问
	void* q_mem[MAX_QUEUE_ENTRIES];//邮箱环形缓冲区，一个指针数组
	u32_t head,tail;               //缓冲区头部和尾部标志
	u32_t msg_num;                 //缓冲区中的消息数量
};

typedef struct lwip_mbox sys_mbox_t;   //定义邮箱类型

#define  SYS_MBOX_NULL   NULL         //定义邮箱类型指针空值

//判断mbox指向的邮箱是否有效
#define sys_mbox_valid(mbox) ((mbox != NULL) && ((mbox)->sem != NULL))
//将mbox指向的邮箱为无效
#define sys_mbox_set_invalid(mbox) ((mbox)->sem = NULL)

typedef INT8U sys_thread_t;    //系统任务标识
#endif
#endif
