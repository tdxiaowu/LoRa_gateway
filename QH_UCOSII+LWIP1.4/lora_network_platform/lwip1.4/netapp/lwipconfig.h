/*
 * lwipconfig.h
 *
 *  Created on: Jan 28, 2019
 *      Author: zm
 */

#ifndef NETAPP_LWIPCONFIG_H_
#define NETAPP_LWIPCONFIG_H_
#include "lwip/opt.h"

void lwip_task(void *pdata);
void lwip_init_task(void);
void lwip_comm_dhcp_creat(void);
#endif /* LWIP1_4_NETAPP_LWIPCONFIG_H_ */
