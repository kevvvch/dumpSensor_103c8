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



#define CH4_R0		(float) (37/10)		//KOhm
#define CH4_RL		(float) 22			//KOhm

#define CH4_P1_X	(float) 200
#define CH4_P1_Y	(float) 2.05
#define CH4_P2_X	(float) 10000
#define CH4_P2_Y	(float) 0.7


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
