/*
 * ch4SensorManager.h
 *
 *  Created on: Nov 30, 2022
 *      Author: kevin
 */

#ifndef INC_CH4SENSORMANAGER_H_
#define INC_CH4SENSORMANAGER_H_

#include "main.h"

/*CALLBACK EVENTS*/
typedef enum {
	__ch4SensorEvent_okMeasuring = 0
} _ch4Sensor_event;




void ch4Sensor_init(ADC_HandleTypeDef *hadc);
void ch4Sensor_handler(void);
void ch4Sensor_measure(void);
uint8_t ch4Sensor_isMeasuring(void);
void ch4Sensor_setCallback(void (*cb)(_ch4Sensor_event, void*));
void ch4Sensor_powerOn(void);
void ch4Sensor_powerOff(void);



#endif /* INC_CH4SENSORMANAGER_H_ */
