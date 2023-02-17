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



#define NH3_R0		(float) (37/3.6)	//KOhm
#define NH3_RL		(float) 22			//KOhm

#define NH3_P1_X	(float) 10
#define NH3_P1_Y	(float) 2.6
#define NH3_P2_X	(float) 200
#define NH3_P2_Y	(float) 0.76


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
