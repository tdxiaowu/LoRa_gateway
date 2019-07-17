/*
 * spi.h
 *
 *  Created on: Apr 20, 2018
 *      Author: zm
 */

#ifndef __SPI_H_
#define __SPI_H_

#include "stm32f10x.h"


void SPI1_Init(void);		/* SPI1初始化配置 */
uint8_t SPI1_ReadWrite(uint8_t writedat);    /**/

#endif /* ENC28J60_SPI_H_ */
