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



#define NH3_R0		(float) 6.3	//KOhm
#define NH3_RL		(float) 22		//KOhm

#define NH3_COEF_A	(float) 1
#define NH3_COEF_B	(float) 0.41
#define NH3_COEF_C	(float) -0.41


void nh3Sensor_init(ADC_HandleTypeDef *hadc);
void nh3Sensor_handler(void);
void nh3Sensor_measure(void);
uint8_t nh3Sensor_isMeasuring(void);
void nh3Sensor_setCallback(void (*cb)(_nh3Sensor_event, void*));
void nh3Sensor_powerOn(void);
void nh3Sensor_powerOff(void);


//Pin Management
#define NH3_SENSOR_VDD_PORT			NH3_VDD_GPIO_Port
#define NH3_SENSOR_VDD_PIN			NH3_VDD_Pin


#endif /* INC_NH3SENSORMANAGER_H_ */
