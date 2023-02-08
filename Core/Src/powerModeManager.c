/*
 * powerModeManager.c
 *
 *  Created on: May 16, 2022
 *      Author: kevin
 */
#include "powerModeManager.h"

static RTC_HandleTypeDef* powerHrtc;
static RTC_TimeTypeDef rtcTime;
RTC_AlarmTypeDef rtcAlarm;

void powerMode_init(RTC_HandleTypeDef* hrtc)
{
	powerHrtc = hrtc;

	if (__HAL_PWR_GET_FLAG(PWR_FLAG_SB) != RESET) {
		__HAL_PWR_CLEAR_FLAG(PWR_FLAG_SB);
		__HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU);
	}
}

void powerMode_enterStandbyMode(uint32_t sec)
{

	// Ponemos la hora actual a 00:00:00
	HAL_RTC_WaitForSynchro(powerHrtc);
	rtcTime.Hours = 0;
	rtcTime.Minutes = 0;
	rtcTime.Seconds = 0;
	HAL_RTC_SetTime(powerHrtc, &rtcTime, RTC_FORMAT_BCD);

	// Configuramos la alarma a las 00:00:10
	rtcAlarm.Alarm = RTC_ALARM_A;
	rtcAlarm.AlarmTime = rtcTime;
	rtcAlarm.AlarmTime.Seconds = sec;
	HAL_RTC_SetAlarm_IT(powerHrtc, &rtcAlarm, RTC_FORMAT_BCD);

	// Entramos en Standby
	HAL_PWR_EnterSTANDBYMode();
}
