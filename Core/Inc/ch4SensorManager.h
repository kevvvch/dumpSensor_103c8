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



#define CH4_R0		(float) 1.58	//KOhm
#define CH4_RL		(float) 22		//KOhm

#define CH4_COEF_A	(float) 2.3
#define CH4_COEF_B	(float) 0.49
#define CH4_COEF_C	(float) -0.38


void ch4Sensor_init(ADC_HandleTypeDef *hadc);
void ch4Sensor_handler(void);
void ch4Sensor_measure(void);
uint8_t ch4Sensor_isMeasuring(void);
void ch4Sensor_setCallback(void (*cb)(_ch4Sensor_event, void*));
void ch4Sensor_powerOn(void);
void ch4Sensor_powerOff(void);




//Pin Management
#define CH4_SENSOR_VDD_PORT			CH4_VDD_GPIO_Port
#define CH4_SENSOR_VDD_PIN			CH4_VDD_Pin


#endif /* INC_CH4SENSORMANAGER_H_ */
