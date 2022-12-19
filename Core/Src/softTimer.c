/* INCLUDES */
#include "softTimer.h"



/* VARIABLES */
static TIM_HandleTypeDef *timerHtim;
static uint32_t systemTicks;





void softTimer_init(TIM_HandleTypeDef *htim)
{
	timerHtim = htim;
	HAL_TIM_Base_Start_IT(htim);
}

void softTimer_start(SoftTimer_t* timer, uint32_t interval)
{
	timer->state = state_running;
	timer->start = softTimer_getTicks();
	timer->interval = interval;
}

uint8_t softTimer_expired(SoftTimer_t* timer)
{
	uint8_t ret = 0;

	if(timer->state == state_running) {
		if(systemTicks >= timer->start)
			ret = ((systemTicks - timer->start) > timer->interval) ? 1 : 0;
		else
			ret = (((0xFFFFFFFF - timer->start)+systemTicks) > timer->interval) ? 1 : 0;
	}

	return ret;
}

void softTimer_stop(SoftTimer_t* timer)
{
	timer->state = state_stopped;
}

void softTimer_handler(void)
{
	systemTicks++;
}

uint32_t softTimer_getTicks(void)
{
	return systemTicks;
}





void softTimer_periodElapsedCallback(TIM_HandleTypeDef *htim)
{
	if(htim->Instance == timerHtim->Instance) {
		softTimer_handler();
	}
}
