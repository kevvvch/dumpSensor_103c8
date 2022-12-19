/*
 * usSensor_driver.h
 *
 *  Created on: Apr 30, 2022
 *      Author: kevin
 */

#ifndef INC_USSENSORMANAGER_H_
#define INC_USSENSORMANAGER_H_

#include "main.h"

/*DEFINES*/
//Sensor specs
#define US_SENSOR_DISTANCE_MAX_MILIMETER	6000
#define US_SENSOR_DISTANCE_MIN_MILIMETER	200
//Temperature
#define US_SENSOR_DEFAULT_TEMPERATURE		20
//Pin Management
#define US_SENSOR_VDD_PORT			2	//TODO Define
#define US_SENSOR_VDD_PIN			3	//TODO Define
#define US_SENSOR_TRIGGER_PORT		US_TRIGGER_GPIO_Port
#define US_SENSOR_TRIGGER_PIN		US_TRIGGER_Pin
#define US_SENSOR_ECHO_TIM_CHANNEL	TIM_CHANNEL_1

/*CALLBACK EVENTS*/
typedef enum {
	__usSensorEvent_okMeasuring = 0,
	__usSensorEvent_errorEcho
} _usSensor_event;

/*PUBLIC FUNCTIONS*/
void usSensor_init(TIM_HandleTypeDef *htim);
void usSensor_handler(void);
void usSensor_powerOn(void);
void usSensor_powerOff(void);
void usSensor_measure(float temp);
uint8_t usSensor_isMeasuring(void);
void usSensor_setCallback(void (*cb)(_usSensor_event, void*));



#endif /* INC_USSENSORMANAGER_H_ */
