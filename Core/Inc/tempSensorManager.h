/*
 * tempSensorManager.h
 *
 *  Created on: Sep 2, 2022
 *      Author: kevin
 */

#ifndef INC_TEMPSENSORMANAGER_H_
#define INC_TEMPSENSORMANAGER_H_

#include "main.h"

/*CALLBACK EVENTS*/
typedef enum {
	__tempSensorEvent_okMeasuring = 0
} _tempSensor_event;




void tempSensor_init(ADC_HandleTypeDef *hadc);
void tempSensor_handler(void);
void tempSensor_measure(void);
uint8_t tempSensor_isMeasuring(void);
void tempSensor_setCallback(void (*cb)(_tempSensor_event, void*));

#endif /* INC_TEMPSENSORMANAGER_H_ */
