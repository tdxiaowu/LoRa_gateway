工程内容：ucosII v2.92.11 + Lwip1.4.1移植
硬件：启航号　STM32F103VET6  +  ENC28J60网卡
１　各文件夹说明：
	bsp/:开发板硬件驱动；
	libraries/:stm32标准开发库3.5，启动文件，链接文件
	lwip1.4/lwip网络协议栈相关的文件：netapp／：网络协议栈应用程序（基于raw接口的），
											lwipconfig.c是协议栈初始化文件
								src/协议栈源文件，其中arch/是移植的文件，包括网卡驱动，通信接口，lwipopt.h是协议栈功能配置文件
	newlib／是linux系统接口文件，用于实现printf函数的重定向
	os_app/ucos操作系统应用程序，包含任务的初始化
	ucosii/操作系统源文件
	user／用户主程序
	
	debug/为程序编译后的文件
	
	
	疑问：
	
	１．在sys_arch.c的sys_sem_new函数中*sem = (void *)new_sem;//?为什么用void
	2:#define MEM_SIZE                     1024*20    //定义内存堆大小?　　如何判断使用多大的内存堆
	
	备忘：暂用.bss较大的区域：ucoss中各任务的栈区，各控制块，内存分配；lwip中内存分配，tcpip进程栈，消息队列个数，要对静态内存区域的合理使用．
	
	
	
	说明：该工程的lwip支持带操作系统模拟层的应用，使用的操作系统ucosii
	也支持无操作系统lwip裸跑的应用，只需要修改lwipopt.h中，修改#define NO_SYS即可，
	方便学习
	
	注意：操作系统模拟层的实现是根据信号量机制来实现的，不能在中断程序中调用模拟层中的邮箱操作函数，中断程序可能阻塞在邮箱的互斥量上等待其他进程释放资源．
	在中断处理程序中，操作系统是禁止进程切换的，这将导致中断程序永远挂起，死锁出现．
	
	2019.3.14 增加了dhcp任务，自动获取ip地址后，将自身删除
	
	2019.3.18 在app_task.c中void AppShowAddress(void *p_arg)//该任务的调用会引起硬件中断，不明原因