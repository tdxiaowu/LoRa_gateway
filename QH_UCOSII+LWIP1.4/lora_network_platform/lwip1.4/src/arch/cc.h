/*
 * cc.h
 *
 *  Created on: Apr 20, 2018
 *      Author: zm
 */

#ifndef __CC_H_
#define __CC_H_

#include "stdio.h"
#include "os_cpu.h"
//定义平台无关的数据类型
typedef  unsigned char   u8_t;    // 8位无符号整数
typedef  signed char     s8_t;    // 8位带符号整数
typedef  unsigned short  u16_t;   // 16位无符号整数
typedef  signed short    s16_t;   // 16位带符号整数
typedef  unsigned long   u32_t;   // 32位无符号整数
typedef  signed long     s32_t;   // 32位带符号整数
typedef  unsigned int    sys_prot_t;  //临界保护型数据
typedef  unsigned int    mem_ptr_t;  //内存地址型数据

/*************cpu 存放数据定义为小端模式******************/
#ifndef BYTE_ORDER
#define BYTE_ORDER LITTLE_ENDIAN
#endif
/*********使用LWIP内部自带的错误代码***************/
#define LWIP_PROVIDE_ERRNO
 /**********定义同结构体封装相关的宏************/
//#if defined(__arm__) && defined(__ARMCC_VERSION)
    #define PACK_STRUCT_BEGIN __packed //__packed 是编译器中限定字节对齐的关键字
    #define PACK_STRUCT_STRUCT
    #define PACK_STRUCT_END
    #define PACK_STRUCT_FIELD(x) x
/*---define (sn)printf formatters for these lwip types, for lwip DEBUG/STATS--*/
/*定义与输出信息格式控制相关的宏*/
#define U16_F "u"  //无符号10进制的形式输出16位无符号整数
#define S16_F "d"  //带符号10进制的形式输出16位带符号整数
#define X16_F "x"  //无符号16进制的形式输出16位号整数
#define U32_F "u"  //无符号10进制的形式输出32位无符号整数
#define S32_F "d"  //带符号10进制的形式输出32位带符号整数
#define X32_F "x"  //无符号16进制的形式输出32位号整数
//定义调试信息输出宏，在debug.h中使用
#define LWIP_DEBUG       //使内核调试信息输出宏LWIP_DEBUGF有效

#ifndef LWIP_PLATFORM_ASSERT  // 被LWIP_ASSERT宏使用
#define LWIP_PLATFORM_ASSERT(x) {printf(x);while(1);}
#endif

#ifndef LWIP_PLATFORM_DIAG  // 被LWIP_DEBUGF宏使用
#define LWIP_PLATFORM_DIAG(x)  do {printf x;} while(0)
#endif

#define LWIP_ERROR(message, expression, handler)  \
	do { if (!(expression)) { \
  			printf(message); handler;}} while(0)


//定义保护宏
#define	SYS_ARCH_DECL_PROTECT(x)	    u32_t cpu_sr

#define	SYS_ARCH_PROTECT(x)			    OS_ENTER_CRITICAL()
#define	SYS_ARCH_UNPROTECT(x)		    OS_EXIT_CRITICAL()

//extern unsinged int sys_now(void);
#endif /* ARCH_CC_H_ */
