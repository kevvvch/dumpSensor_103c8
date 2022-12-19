/*
 * softTimer.h
 *
 *  Created on: May 1, 2022
 *      Author: kevin
 */

#ifndef INC_SOFTTIMER_H_
#define INC_SOFTTIMER_H_

#include "main.h"



typedef enum {
	state_running = 0,
	state_stopped
} stateSoftTimer;

typedef struct {
	uint32_t start;
	uint32_t interval;
	stateSoftTimer state;
} SoftTimer_t;



void softTimer_init(TIM_HandleTypeDef *htim);
void softTimer_start(SoftTimer_t* timer, uint32_t interval);
uint8_t softTimer_expired(SoftTimer_t* timer);
void softTimer_stop(SoftTimer_t* timer);
void softTimer_handler(void);
uint32_t softTimer_getTicks(void);
void softTimer_periodElapsedCallback(TIM_HandleTypeDef *htim);



#endif /* INC_SOFTTIMER_H_ */
