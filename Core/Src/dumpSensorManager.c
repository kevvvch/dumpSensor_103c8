/*
 * dumpSensorManager.c
 *
 *  Created on: Nov 29, 2022
 *      Author: kevin
 */

#include "fsmManager.h"
#include "dumpSensorManager.h"
#include "powerModeManager.h"
#include "softTimer.h"
#include "usSensorManager.h"
#include "nh3SensorManager.h"
#include "ch4SensorManager.h"
#include "tempSensorManager.h"
#include "gsmModuleManager.h"
#include "utilities.h"
#include "nvmManager.h"
#include "project.h"

/* EXTERN VARIABLES */
extern RTC_HandleTypeDef hrtc;
extern TIM_HandleTypeDef htim3;
extern TIM_HandleTypeDef htim4;
extern ADC_HandleTypeDef hadc1;
extern UART_HandleTypeDef huart1;

/* PRIVATE VARIABLES */
/* FLAGS */
typedef union {
	uint32_t dword;
	struct {
		uint8_t battery_measureDone:1;
		uint8_t tempSensor_measureDone:1;
		uint8_t usSensor_measureDone:1;
		uint8_t nh3Sensor_measureDone:1;
		uint8_t ch4Sensor_measureDone:1;
		uint8_t gsmModule_sendPackageDone:1;
		uint8_t gsmModule_turnOn;
		uint8_t gsmModule_getGps;
	} bits;
} _flags_dumpSensor;

_flags_dumpSensor flags_dumpSensor;


//NVM
typedef union {
    uint16_t word;
    struct {
        uint8_t bit0:1;
        uint8_t bit1:1;
        uint8_t bit2:1;
        uint8_t bit3:1;
        uint8_t bit4:1;
        uint8_t bit5:1;
        uint8_t bit6:1;
        uint8_t bit7:1;
        uint8_t bit8:1;
        uint8_t bit9:1;
        uint8_t bit10:1;
        uint8_t bit11:1;
        uint8_t bit12:1;
        uint8_t bit13:1;
        uint8_t bit14:1;
        uint8_t bit15:1;
    } bits;
} nvmReg;

nvmReg reg[NVM_BACKUPREGISTERS_NUMBER];

static uint32_t productId;

static float distance;			//mm
static float nh3Concentration;	//ppm
static float ch4Concentration;	//ppm
static float temperature;		//Celcius
static float battery;			//Celcius
static uint8_t gspLon[15];
static uint8_t gspLat[15];

//static float distancePercent;
static float batteryPercent;
static uint8_t payloadDataToSend[150];	//Data to send to  server
static uint8_t auxToSend[50];	//Auxiliar variable

/* TIMERS */
static SoftTimer_t timer;

/* FSM */
typedef enum {
	__dumpSensor_idle,
	__dumpSensor_readNvm,
	__dumpSensor_measureBattery,
	__dumpSensor_measureTemperature,
	__dumpSensor_measureLevel,
	__dumpSensor_heatGasSensor,
	__dumpSensor_measureNh3,
	__dumpSensor_measureCh4,
	__dumpSensor_getGps,
	__dumpSensor_turnOffGps,
	__dumpSensor_sendPackage,
	__dumpSensor_disconnectServer,
	__dumpSensor_turnOffGsmModule,
	__dumpSensor_sleep,
	__dumpSensor_error
} _dumpSensor_state;
fsm_t dumpSensorFsmState;


/* PRIVATE FUNCTIONS */
static void readNvm(void);
static void writeNvm(void);
static float calculateDistancePercentage(float dist);



void usSensorCb(_usSensor_event evt, void* payload)
{
	switch(evt)
	{
		case __usSensorEvent_okMeasuring:;
			distance = *(float*) payload;

			flags_dumpSensor.bits.usSensor_measureDone = 1;
			break;

		case __usSensorEvent_errorEcho:;

			break;
	}
}

void nh3SensorCb(_nh3Sensor_event evt, void* payload)
{
	switch(evt)
	{
		case __nh3SensorEvent_okMeasuring:;
			nh3Concentration = *(float*) payload;

			flags_dumpSensor.bits.nh3Sensor_measureDone = 1;
			break;
	}
}

void ch4SensorCb(_ch4Sensor_event evt, void* payload)
{
	switch(evt)
	{
		case __ch4SensorEvent_okMeasuring:;
			ch4Concentration = *(float*) payload;

			flags_dumpSensor.bits.ch4Sensor_measureDone = 1;
			break;
	}
}

void tempSensorCb(_tempSensor_event evt, void* payload)
{
	switch(evt)
	{
		case __tempSensorEvent_okMeasuring:;
			temperature = *(float*) payload;
			temperature -= 10;	//Subtract addicional temperature of the capsule

			flags_dumpSensor.bits.tempSensor_measureDone = 1;
			break;
	}
}

void gsmModuleCb(_gsmModule_event evt, void* payload)
{
	switch(evt)
	{
		case __gsmModuleEvent_okGpsInfo:;
			uint8_t *gspInfo = (uint8_t*) payload;
			uint8_t strOut[10];

			string_split(gspInfo, ',', strOut);
			string_split(gspInfo, ',', strOut);
			string_split(gspInfo, ',', gspLon);
			string_split(gspInfo, ',', strOut);
			string_split(gspInfo, ',', gspLat);

			flags_dumpSensor.bits.tempSensor_measureDone = 1;
			break;
	}
}


void dumpSensorManager_init(void)
{
	//Wakes up from sleep
	powerMode_init(&hrtc);

	//Initializes softTimer
	softTimer_init(&htim4);

	//Initializes ultrasonic Sensor
	usSensor_init(&htim3);
	usSensor_setCallback(usSensorCb);
	usSensor_powerOff();

	//Initializes temperature Sensor
	tempSensor_init(&hadc1);
	tempSensor_setCallback(tempSensorCb);

	//Initializes NH3 Sensor
	nh3Sensor_init(&hadc1);
	nh3Sensor_setCallback(nh3SensorCb);
	nh3Sensor_powerOff();

	//Initializes CH4 Sensor
	ch4Sensor_init(&hadc1);
	ch4Sensor_setCallback(ch4SensorCb);
	ch4Sensor_powerOff();

	//Initializes GSM Module
	gsmModule_init(&huart1);
	gsmModule_powerOff();
	gsmModule_setCallback(gsmModuleCb);

	//Initializes NVM managment Module
	nvm_init(&hrtc);

	//FSM
	fsmManager_init(&dumpSensorFsmState, __dumpSensor_idle);

	//Flags
	flags_dumpSensor.dword = 0;
}

void dumpSensorManager_handler(void)
{
	usSensor_handler();
	tempSensor_handler();
	nh3Sensor_handler();
	ch4Sensor_handler();
	gsmModule_handler();



	if(flags_dumpSensor.bits.gsmModule_turnOn == 1) {
		flags_dumpSensor.bits.gsmModule_turnOn = 0;

		gsmModule_powerOn();
		gsmModule_pwrkeyOn();
	}

	switch(fsmManager_getState(&dumpSensorFsmState))
	{
		case __dumpSensor_idle:
			if(fsmManager_isStateIn(&dumpSensorFsmState)) {
				fsmManager_stateIn(&dumpSensorFsmState);

#if defined(STATUS_LED)
				HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, 1);
#endif
			}

			fsmManager_gotoState(&dumpSensorFsmState, __dumpSensor_readNvm);

			if(fsmManager_isStateOut(&dumpSensorFsmState)) {
				fsmManager_stateOut(&dumpSensorFsmState);
			}
			break;



		case __dumpSensor_readNvm:
			if(fsmManager_isStateIn(&dumpSensorFsmState)) {
				fsmManager_stateIn(&dumpSensorFsmState);
			}

			readNvm();
			writeNvm();

			//Gets product ID
			productId = (((uint32_t) reg[NVM_ID_HIGH].word) << 16) | reg[NVM_ID_LOW].word;

			//Gets flags


			fsmManager_gotoState(&dumpSensorFsmState, __dumpSensor_measureBattery);

			if(fsmManager_isStateOut(&dumpSensorFsmState)) {
				fsmManager_stateOut(&dumpSensorFsmState);
			}
			break;



		case __dumpSensor_measureBattery:
			if(fsmManager_isStateIn(&dumpSensorFsmState)) {
				fsmManager_stateIn(&dumpSensorFsmState);
			}

			//Gets battery level percetange
			battery = 0.75;
			batteryPercent = battery*100;
			/*
			if(batteryPercent < DUMPSTER_BATTERY_PERC_TRIG_NEG || ) {
				flags_dumpSensor.bits.gsmModule_turnOn = 1;
			}
			*/

			fsmManager_gotoState(&dumpSensorFsmState, __dumpSensor_measureTemperature);


			if(fsmManager_isStateOut(&dumpSensorFsmState)) {
				fsmManager_stateOut(&dumpSensorFsmState);
			}
			break;



		case __dumpSensor_measureTemperature:
			if(fsmManager_isStateIn(&dumpSensorFsmState)) {
				fsmManager_stateIn(&dumpSensorFsmState);

				tempSensor_measure();
			}

			if(flags_dumpSensor.bits.tempSensor_measureDone == 1) {
				fsmManager_gotoState(&dumpSensorFsmState, __dumpSensor_measureLevel);
				flags_dumpSensor.bits.gsmModule_turnOn = 1;
			}

			if(fsmManager_isStateOut(&dumpSensorFsmState)) {
				fsmManager_stateOut(&dumpSensorFsmState);
			}
			break;



		case __dumpSensor_measureLevel:
			if(fsmManager_isStateIn(&dumpSensorFsmState)) {
				fsmManager_stateIn(&dumpSensorFsmState);

				usSensor_powerOn();
				usSensor_measure(temperature);

				softTimer_start(&timer, 10*1000);
			}

			if(flags_dumpSensor.bits.usSensor_measureDone == 1) {
#if defined PROJECT_NOLOGIC
				fsmManager_gotoState(&dumpSensorFsmState, __dumpSensor_heatGasSensor);

#else

				if(distancePercent > DUMPSTER_DISTANCE_PERC_TRIG_POS && reg[NVM_FLAGS0].bits.bit1 == 0) {
					reg[NVM_FLAGS0].bits.bit1 = 1;
					nvm_writeBit(NVM_FLAGS0, 1, (uint32_t) reg[NVM_FLAGS0].bits.bit1);

					flags_dumpSensor.bits.gsmModule_turnOn = 1;
					fsmManager_gotoState(&dumpSensorFsmState, __dumpSensor_heatGasSensor);
				}
				else if(distancePercent < DUMPSTER_DISTANCE_PERC_TRIG_NEG && reg[NVM_FLAGS0].bits.bit1 == 1) {
					reg[NVM_FLAGS0].bits.bit1 = 0;
					nvm_writeBit(NVM_FLAGS0, 1, (uint32_t) reg[NVM_FLAGS0].bits.bit1);

					flags_dumpSensor.bits.gsmModule_turnOn = 1;
					fsmManager_gotoState(&dumpSensorFsmState, __dumpSensor_heatGasSensor);
				}
				else {
					fsmManager_gotoState(&dumpSensorFsmState, __dumpSensor_sleep);
				}
#endif
			}
			else if(softTimer_expired(&timer)) {
				fsmManager_gotoState(&dumpSensorFsmState, __dumpSensor_heatGasSensor);
			}

			if(fsmManager_isStateOut(&dumpSensorFsmState)) {
				fsmManager_stateOut(&dumpSensorFsmState);

				usSensor_powerOff();
			}
			break;

		case __dumpSensor_heatGasSensor:
			if(fsmManager_isStateIn(&dumpSensorFsmState)) {
				fsmManager_stateIn(&dumpSensorFsmState);

				ch4Sensor_powerOn();
				nh3Sensor_powerOn();

				softTimer_start(&timer, 20*1000);
			}

			if(softTimer_expired(&timer)) {
				fsmManager_gotoState(&dumpSensorFsmState, __dumpSensor_measureNh3);
			}

			if(fsmManager_isStateOut(&dumpSensorFsmState)) {
				fsmManager_stateOut(&dumpSensorFsmState);
			}
			break;

		case __dumpSensor_measureNh3:
			if(fsmManager_isStateIn(&dumpSensorFsmState)) {
				fsmManager_stateIn(&dumpSensorFsmState);

				nh3Sensor_measure();

				softTimer_start(&timer, 10*1000);
			}

			if(flags_dumpSensor.bits.nh3Sensor_measureDone == 1) {
				fsmManager_gotoState(&dumpSensorFsmState, __dumpSensor_measureCh4);
			}
			else if(softTimer_expired(&timer)) {
				//If there is not a measurement within 500 mseg, stop trying to measure
				fsmManager_gotoState(&dumpSensorFsmState, __dumpSensor_measureCh4);
			}

			if(fsmManager_isStateOut(&dumpSensorFsmState)) {
				fsmManager_stateOut(&dumpSensorFsmState);

				nh3Sensor_powerOff();
			}
			break;



		case __dumpSensor_measureCh4:
			if(fsmManager_isStateIn(&dumpSensorFsmState)) {
				fsmManager_stateIn(&dumpSensorFsmState);

				ch4Sensor_measure();

				softTimer_start(&timer, 10*1000);
			}

			if(flags_dumpSensor.bits.ch4Sensor_measureDone == 1) {
				fsmManager_gotoState(&dumpSensorFsmState, __dumpSensor_getGps);
			}
			else if(softTimer_expired(&timer)) {
				//If there is not a measurement within 500 mseg, stop trying to measure
				fsmManager_gotoState(&dumpSensorFsmState, __dumpSensor_getGps);
			}

			if(fsmManager_isStateOut(&dumpSensorFsmState)) {
				fsmManager_stateOut(&dumpSensorFsmState);

				ch4Sensor_powerOff();
			}
			break;



		case __dumpSensor_getGps:
			if(fsmManager_isStateIn(&dumpSensorFsmState)) {
				fsmManager_stateIn(&dumpSensorFsmState);

				gsmModule_gpsOn();
				softTimer_start(&timer, 20*60*1000);
			}

			if(gsmModule_isGpsFixed()) {
				fsmManager_gotoState(&dumpSensorFsmState, __dumpSensor_turnOffGps);
			}
			else if(softTimer_expired(&timer)) {
				fsmManager_gotoState(&dumpSensorFsmState, __dumpSensor_turnOffGps);
			}
			else if(gsmModule_isPowered() && gsmModule_isGpsOn() && !gsmModule_isGpsFixed() && !gsmModule_requestedGpsInfo()) {
				gsmModule_gpsInfo();
			}

			if(fsmManager_isStateOut(&dumpSensorFsmState)) {
				fsmManager_stateOut(&dumpSensorFsmState);
			}
			break;



		case __dumpSensor_turnOffGps:
			if(fsmManager_isStateIn(&dumpSensorFsmState)) {
				fsmManager_stateIn(&dumpSensorFsmState);

				gsmModule_gpsOff();
				softTimer_start(&timer, 60*1000);
			}

			if(!gsmModule_isGpsOn()) {
				fsmManager_gotoState(&dumpSensorFsmState, __dumpSensor_sendPackage);
			}
			else if(softTimer_expired(&timer)) {
				fsmManager_gotoState(&dumpSensorFsmState, __dumpSensor_sendPackage);
			}

			if(fsmManager_isStateOut(&dumpSensorFsmState)) {
				fsmManager_stateOut(&dumpSensorFsmState);
			}
			break;



		case __dumpSensor_sendPackage:
			if(fsmManager_isStateIn(&dumpSensorFsmState)) {
				fsmManager_stateIn(&dumpSensorFsmState);

				gsmModule_serverConnect();

				payloadDataToSend[0] = '\0';
				string_appendString(payloadDataToSend, (uint8_t *) "id=");
				number_convertUint32_toArrayOfUint8(auxToSend, productId);
				string_appendString(payloadDataToSend, auxToSend);
				string_appendChar(payloadDataToSend, '&');

				string_appendString(payloadDataToSend, (uint8_t *) "bat=");
				ascii_convertNum(auxToSend, (uint32_t) batteryPercent);
				string_appendString(payloadDataToSend, auxToSend);

				if(flags_dumpSensor.bits.usSensor_measureDone) {
					string_appendChar(payloadDataToSend, '&');
					string_appendString(payloadDataToSend, (uint8_t *) "level=");
					ascii_convertNum(auxToSend, (uint32_t) distance);
					string_appendString(payloadDataToSend, auxToSend);
				}

				if(flags_dumpSensor.bits.nh3Sensor_measureDone) {
					string_appendChar(payloadDataToSend, '&');
					string_appendString(payloadDataToSend, (uint8_t *) "nh3=");
					ascii_convertNum(auxToSend, (uint32_t) nh3Concentration);
					string_appendString(payloadDataToSend, auxToSend);
				}

				if(flags_dumpSensor.bits.ch4Sensor_measureDone) {
					string_appendChar(payloadDataToSend, '&');
					string_appendString(payloadDataToSend, (uint8_t *) "ch4=");
					ascii_convertNum(auxToSend, (uint32_t) ch4Concentration);
					string_appendString(payloadDataToSend, auxToSend);
				}

				if(gsmModule_isGpsFixed()) {
					string_appendChar(payloadDataToSend, '&');
					string_appendString(payloadDataToSend, (uint8_t *) "gpslon=");
					string_appendString(payloadDataToSend, gspLon);

					string_appendChar(payloadDataToSend, '&');
					string_appendString(payloadDataToSend, (uint8_t *) "gpslat=");
					string_appendString(payloadDataToSend, gspLat);
				}
				string_appendChar(payloadDataToSend, '"');

				softTimer_start(&timer, 10*60*1000);
			}


			if(gsmModule_isServerDataSent()) {
				fsmManager_gotoState(&dumpSensorFsmState, __dumpSensor_disconnectServer);
			}
			else if(softTimer_expired(&timer) || gsmModule_errorServerSendData()) {
				fsmManager_gotoState(&dumpSensorFsmState, __dumpSensor_disconnectServer);
			}
			else if(gsmModule_isPowered() && gsmModule_isServerConnected() && !gsmModule_isServerDataSent() && !gsmModule_requestedServerDataSend()) {
				gsmModule_serverDataSend(payloadDataToSend);
			}

			if(fsmManager_isStateOut(&dumpSensorFsmState)) {
				fsmManager_stateOut(&dumpSensorFsmState);
			}
			break;



		case __dumpSensor_disconnectServer:
			if(fsmManager_isStateIn(&dumpSensorFsmState)) {
				fsmManager_stateIn(&dumpSensorFsmState);

				gsmModule_serverDisconnect();

				softTimer_start(&timer, 5*1000);
			}

			if(gsmModule_isServerConnected() == 0) {
				fsmManager_gotoState(&dumpSensorFsmState, __dumpSensor_turnOffGsmModule);
			}
			else if(softTimer_expired(&timer)) {
				fsmManager_gotoState(&dumpSensorFsmState, __dumpSensor_turnOffGsmModule);
			}

			if(fsmManager_isStateOut(&dumpSensorFsmState)) {
				fsmManager_stateOut(&dumpSensorFsmState);
			}
			break;



		case __dumpSensor_turnOffGsmModule:
			if(fsmManager_isStateIn(&dumpSensorFsmState)) {
				fsmManager_stateIn(&dumpSensorFsmState);

				gsmModule_powerOff();
				gsmModule_pwrkeyOff();

				softTimer_start(&timer, 10*1000);
			}

			if(gsmModule_isPowered() == 0) {
				fsmManager_gotoState(&dumpSensorFsmState, __dumpSensor_sleep);
			}
			else if(softTimer_expired(&timer)) {
				fsmManager_gotoState(&dumpSensorFsmState, __dumpSensor_sleep);
			}

			if(fsmManager_isStateOut(&dumpSensorFsmState)) {
				fsmManager_stateOut(&dumpSensorFsmState);
			}
			break;



		case __dumpSensor_sleep:
			if(fsmManager_isStateIn(&dumpSensorFsmState)) {
				fsmManager_stateIn(&dumpSensorFsmState);
			}

			//Enters standby mode
			powerMode_enterStandbyMode(1*30);

			fsmManager_gotoState(&dumpSensorFsmState, __dumpSensor_idle);

			if(fsmManager_isStateOut(&dumpSensorFsmState)) {
				fsmManager_stateOut(&dumpSensorFsmState);
			}
			break;



		case __dumpSensor_error:
			if(fsmManager_isStateIn(&dumpSensorFsmState)) {
				fsmManager_stateIn(&dumpSensorFsmState);
			}

			fsmManager_gotoState(&dumpSensorFsmState, __dumpSensor_idle);

			if(fsmManager_isStateOut(&dumpSensorFsmState)) {
				fsmManager_stateOut(&dumpSensorFsmState);
			}
			break;



		default:
			if(fsmManager_isStateIn(&dumpSensorFsmState)) {
				fsmManager_stateIn(&dumpSensorFsmState);
			}

			fsmManager_gotoState(&dumpSensorFsmState, __dumpSensor_idle);

			if(fsmManager_isStateOut(&dumpSensorFsmState)) {
				fsmManager_stateOut(&dumpSensorFsmState);
			}
			break;
	}
}

static float calculateDistancePercentage(float dist)
{
	float distPerc = 0;

	distPerc = (DUMPSTER_DISTANCE_MAX-distance)*100/DUMPSTER_DISTANCE_MAX;
	if(distPerc > 100) {
		distPerc = 100;
	}
	else if(distPerc < 0) {
		distPerc = 0;
	}

	return distPerc;
}

static void readNvm(void)
{
	for(uint8_t i = 0; i < NVM_BACKUPREGISTERS_NUMBER; i++) {
		reg[i].word = nvm_readWord(NVM_REGISTER1+i);
	}
}

//https://docs.google.com/spreadsheets/d/1n-ANLwi3L4sAhEEs7HliKyX1nLDQ-HLQLX-bY1MBAcw/edit#gid=0
static void writeNvm(void)
{
	//If it is needed to write to nvm, toggle the value of reg[NVM_FLAGS0].bits.bit0
	if(reg[NVM_FLAGS0].bits.bit0 != 1) {
		reg[NVM_FLAGS0].bits.bit0 = 1;

		reg[NVM_ID_HIGH].word = 0xAE23;
		reg[NVM_ID_LOW].word = 0xFA5B;

		reg[NVM_COUNTER_DAY].word = 0;

		reg[NVM_FLAGS0].bits.bit1 = 0;
		reg[NVM_FLAGS0].bits.bit2 = 0;

		for(uint8_t i = 0; i < NVM_BACKUPREGISTERS_NUMBER; i++) {
			nvm_writeWord(NVM_REGISTER1+i, reg[i].word);
		}
	}
}
