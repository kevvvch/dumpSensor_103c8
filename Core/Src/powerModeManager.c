/*
 * powerModeManager.c
 *
 *  Created on: May 16, 2022
 *      Author: kevin
 */
#include "powerModeManager.h"

static RTC_HandleTypeDef* powerHrtc;

/*
void powerMode_init(RTC_HandleTypeDef* hrtc)
{
	powerHrtc = hrtc;

	if(__HAL_PWR_GET_FLAG(PWR_FLAG_SB) != RESET) {
		//Clear standby flag
		__HAL_PWR_CLEAR_FLAG(PWR_FLAG_SB);

		//Disable wakeup pin
		HAL_PWR_DisableWakeUpPin(PWR_WAKEUP_PIN1);

		//Deactivate RTC wakeup
		HAL_RTCEx_DeactivateWakeUpTimer(hrtc);
	}
}

void powerMode_enterStandbyMode(uint32_t sec)
{
	//Clear wakeup flag
	__HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU);

	//Clear RTC wakeup flag
	__HAL_RTC_WAKEUPTIMER_CLEAR_FLAG(powerHrtc, RTC_FLAG_WUTF);

	//Enable wakeup pin before entering standby mode
	HAL_PWR_EnableWakeUpPin(PWR_WAKEUP_PIN1);

	//Enable RTC wakeup
	if (HAL_RTCEx_SetWakeUpTimer_IT(powerHrtc, sec, RTC_WAKEUPCLOCK_CK_SPRE_16BITS) != HAL_OK) {
	  Error_Handler();
	}

	//Enter standby mode
	HAL_PWR_EnterSTANDBYMode();
}
*/
