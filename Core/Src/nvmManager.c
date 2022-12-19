/* INCLUDES */
#include "nvmManager.h"



/* VARIABLES */
static RTC_HandleTypeDef *nvmHrtc;




/* FUNCTION PROTOTYPES */
static void RTC_WriteWord_BackupRegister(uint32_t backupRegister, uint32_t wData);
static uint32_t RTC_Read_BackupRegister(uint32_t backupRegister);





void nvm_init(RTC_HandleTypeDef *hrtc)
{
	//RTC
	nvmHrtc = hrtc;
}

uint32_t nvm_readWord(uint32_t nReg)
{
	return RTC_Read_BackupRegister(nReg);
}

uint32_t nvm_readBit(uint32_t nReg, uint32_t pos)
{
	uint32_t wData = RTC_Read_BackupRegister(nReg);

	return (wData & (1 << pos)) >> pos;
}

void nvm_writeWord(uint32_t nReg, uint32_t wData)
{
	RTC_WriteWord_BackupRegister(nReg, wData);
}

void nvm_writeBit(uint32_t nReg, uint32_t pos, uint32_t bData)
{
	uint32_t wData = RTC_Read_BackupRegister(nReg);
	uint32_t mask;

	//Clears bit in specified position of backup register
	mask = 1 << pos;
	mask = ~mask;
	wData &= mask;

	//Writes bit
	mask = (bData?1:0) << pos;
	wData |= mask;
	RTC_WriteWord_BackupRegister(nReg, wData);
}





static uint32_t RTC_Read_BackupRegister(uint32_t backupRegister)
{
    return HAL_RTCEx_BKUPRead(nvmHrtc, backupRegister);
}

static void RTC_WriteWord_BackupRegister(uint32_t backupRegister, uint32_t wData)
{
    HAL_PWR_EnableBkUpAccess();
    HAL_RTCEx_BKUPWrite(nvmHrtc, backupRegister, (uint16_t) wData);
    HAL_PWR_DisableBkUpAccess();
}
