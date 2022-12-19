/*
 * nh3SensorManager.h
 *
 *  Created on: Nov 30, 2022
 *      Author: kevin
 */

#ifndef INC_NH3SENSORMANAGER_H_
#define INC_NH3SENSORMANAGER_H_

#include "main.h"

/*CALLBACK EVENTS*/
typedef enum {
	__nh3SensorEvent_okMeasuring = 0
} _nh3Sensor_event;




void nh3Sensor_init(ADC_HandleTypeDef *hadc);
void nh3Sensor_handler(void);
void nh3Sensor_measure(void);
uint8_t nh3Sensor_isMeasuring(void);
void nh3Sensor_setCallback(void (*cb)(_nh3Sensor_event, void*));
void nh3Sensor_powerOn(void);
void nh3Sensor_powerOff(void);



#endif /* INC_NH3SENSORMANAGER_H_ */
