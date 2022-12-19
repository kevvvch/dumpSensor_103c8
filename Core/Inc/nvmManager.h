/*
 * nvmManager.h
 *
 *  Created on: Sep 3, 2022
 *      Author: kevin
 */

#ifndef INC_NVMMANAGER_H_
#define INC_NVMMANAGER_H_

#include <main.h>

#define NVM_BACKUPREGISTERS_NUMBER	10
#define NVM_BACKUPREGISTERS_BITS	16

//Para el STM32F103C8 solo hay 10 registros de 16 bits
#define NVM_REGISTER1		RTC_BKP_DR1
#define NVM_REGISTER2		RTC_BKP_DR2
#define NVM_REGISTER3		RTC_BKP_DR3
#define NVM_REGISTER4		RTC_BKP_DR4
#define NVM_REGISTER5		RTC_BKP_DR5
#define NVM_REGISTER6		RTC_BKP_DR6
#define NVM_REGISTER7		RTC_BKP_DR7
#define NVM_REGISTER8		RTC_BKP_DR8
#define NVM_REGISTER9		RTC_BKP_DR9
#define NVM_REGISTER10		RTC_BKP_DR10

void nvm_init(RTC_HandleTypeDef *hrtc);
uint32_t nvm_readWord(uint32_t nReg);
uint32_t nvm_readBit(uint32_t nReg, uint32_t pos);
void nvm_writeWord(uint32_t nReg, uint32_t wData);
void nvm_writeBit(uint32_t nReg, uint32_t pos, uint32_t bData);



#endif /* INC_NVMMANAGER_H_ */
