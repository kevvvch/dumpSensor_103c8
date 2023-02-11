/* INCLUDES */
#include "main.h"
#include "usSensorManager.h"
#include "fsmManager.h"
#include "softTimer.h"
#include "utilities.h"



/* DEFINES */


/* TYPEDEF */
//FSM
typedef enum {
	__usSensor_idle,
	__usSensor_delay,
	__usSensor_pinUsTrigger_write,
	__usSensor_waitEcho,
	__usSensor_getDistance,

	__usSensor_errorWaitEcho,
	__usSensor_errorEchoOverflow,
	__usSensor_error
} _usSensor_state;

//Flags
typedef union {
	uint32_t dword;
	struct {
		uint8_t requestMeasure:1;
		uint8_t bit1:1;
		uint8_t bit2:1;
		uint8_t bit3:1;
		uint8_t bit4:1;
		uint8_t bit5:1;
		uint8_t receivedEcho:1;
		uint8_t bit7:1;
		uint8_t bit8:1;
		uint8_t bit9:1;
		uint8_t isFirstCaptured:1;
		uint8_t isMeasuring:1;
	} bits;
} _flags_usSensor;

typedef union {
	uint32_t dword;
	struct {
		uint8_t echo:1;
		uint8_t bit1:1;
		uint8_t bit2:1;
		uint8_t bit3:1;
		uint8_t bit4:1;
		uint8_t bit5:1;
		uint8_t bit6:1;
		uint8_t bit7:1;
		uint8_t bit8:1;
	} bits;
} _flags_usSensorError;



/* VARIABLES */
//Table: Temperature [ºC] vs Speed of sound [m/s]
static float tableTempVsSpeed[15][2] = {
		{-40, 306.2},
		{0, 331.4},
		{5, 334.4},
		{10, 337.4},
		{15, 340.4},
		{20, 343.3},
		{25, 346.3},
		{30, 349.1},
		{40, 354.7},
		{50, 360.3},
		{60, 365.7},
		{70, 371.2},
		{80, 376.6},
		{90, 381.7},
		{100, 386.9}
};
static float temperature;

//Timers
static TIM_HandleTypeDef *usHtim;
static SoftTimer_t timer;
static SoftTimer_t timeout;

//FSM
static fsm_t usSensor_state;

//Flags
static _flags_usSensor flags_usSensor;
static _flags_usSensorError flags_usSensorError;

//Input capture
volatile static uint32_t icVal1, icVal2, icDif;
static float mFactor;
volatile static float echoTime;			//Echo pulse time [useg]

//Variables
static float usDistance;		//Distance between sensor and obstacle [mm]
static float soundSpeed;		//Speed of sound [m/s]



/* FUNCTION PROTOTYPES */
//Functions
static float getSoundSpeed(void);

//Pin Management
static void pinUsVdd_write(uint8_t state);
static void pinUsTrigger_write(uint8_t state);
static void pinUsEcho_inputCaptureStart(void);
static void pinUsEcho_inputCaptureStop(void);

//Callback
static void (*usSensorCallback)(_usSensor_event evt, void* payload);





void usSensor_init(TIM_HandleTypeDef *htim)
{
	//Timer
	usHtim = htim;

	//FSM
	fsmManager_init(&usSensor_state, __usSensor_idle);

	//Pin
	pinUsVdd_write(0);
	pinUsTrigger_write(0);
	//pinUsEcho_inputCaptureStop();
	//HAL_TIM_IC_Start_IT(usHtim, US_SENSOR_ECHO_TIM_CHANNEL);

	//Flags
	flags_usSensor.dword = 0;
	flags_usSensorError.dword = 0;

	//Variables
	echoTime = 0;
	usDistance = 0;
	soundSpeed = numberFloat_getLinearValue(US_SENSOR_DEFAULT_TEMPERATURE, tableTempVsSpeed, sizeof(tableTempVsSpeed)/(sizeof(float)*2));		//Default 25 celcius

	//Callback
	usSensorCallback = NULL;

	//Reference clock for Input Capture
	mFactor = 1000000*(usHtim->Init.Prescaler+1)/HAL_RCC_GetSysClockFreq();
}

void usSensor_handler(void)
{
	switch(fsmManager_getState(&usSensor_state)) {
		case __usSensor_idle:
			if(fsmManager_isStateIn(&usSensor_state)) {
				fsmManager_stateIn(&usSensor_state);

				//Clear signals
				pinUsTrigger_write(0);

				//Clear variables
				echoTime = 0;
				usDistance = 0;
				flags_usSensor.bits.isMeasuring = 0;
			}

			if(flags_usSensor.bits.requestMeasure) {
				flags_usSensor.bits.requestMeasure = 0;
				flags_usSensor.bits.isMeasuring = 1;

				fsmManager_gotoState(&usSensor_state,__usSensor_delay);
			}

			if(fsmManager_isStateOut(&usSensor_state)) {
				fsmManager_stateOut(&usSensor_state);
			}
			break;



		case __usSensor_delay:
			if(fsmManager_isStateIn(&usSensor_state)) {
				fsmManager_stateIn(&usSensor_state);

				softTimer_start(&timer, 1000);
			}

			if(softTimer_expired(&timer)) {
				fsmManager_gotoState(&usSensor_state,__usSensor_pinUsTrigger_write);
			}

			if(fsmManager_isStateOut(&usSensor_state)) {
				fsmManager_stateOut(&usSensor_state);
			}
			break;



		case __usSensor_pinUsTrigger_write:
			if(fsmManager_isStateIn(&usSensor_state)) {
				fsmManager_stateIn(&usSensor_state);

				//Set trigger during 1 mseg
				pinUsTrigger_write(1);
				softTimer_start(&timer, 10);
			}

			if(softTimer_expired(&timer)) {
				softTimer_stop(&timer);

				//Finished setting trigger
				pinUsTrigger_write(0);

				//Enable input capture for echo
				pinUsEcho_inputCaptureStart();

				fsmManager_gotoState(&usSensor_state,__usSensor_waitEcho);
			}

			if(fsmManager_isStateOut(&usSensor_state)) {
				fsmManager_stateOut(&usSensor_state);
			}
			break;



		case __usSensor_waitEcho:
			if(fsmManager_isStateIn(&usSensor_state)) {
				fsmManager_stateIn(&usSensor_state);

				//Set 100mseg timeout
				softTimer_start(&timeout, 100);
			}

			//Echo received
			if(flags_usSensor.bits.receivedEcho == 1) {
				flags_usSensor.bits.receivedEcho = 0;

				fsmManager_gotoState(&usSensor_state,__usSensor_getDistance);
			} else if(softTimer_expired(&timeout)) {
				softTimer_stop(&timeout);

				fsmManager_gotoState(&usSensor_state,__usSensor_errorWaitEcho);
			}

			if(fsmManager_isStateOut(&usSensor_state)) {
				fsmManager_stateOut(&usSensor_state);

				pinUsEcho_inputCaptureStop();
			}
			break;



		case __usSensor_getDistance:
			if(fsmManager_isStateIn(&usSensor_state)) {
				fsmManager_stateIn(&usSensor_state);
			}

			//Get speed of sound based on temperature
			soundSpeed = getSoundSpeed();

			//Calculate usDistance
			usDistance = (echoTime*soundSpeed/2000); //[mm]

			if(usDistance > US_SENSOR_DISTANCE_MAX_MILIMETER) {
				fsmManager_gotoState(&usSensor_state,__usSensor_errorEchoOverflow);
			} else {
				//Informs to higher layer
				if(usSensorCallback != NULL) {
					usSensorCallback(__usSensorEvent_okMeasuring, (float *) &usDistance);
				}

				fsmManager_gotoState(&usSensor_state,__usSensor_idle);
			}


			if(fsmManager_isStateOut(&usSensor_state)) {
				fsmManager_stateOut(&usSensor_state);
			}
			break;



		case __usSensor_errorWaitEcho:
			if(fsmManager_isStateIn(&usSensor_state)) {
				fsmManager_stateIn(&usSensor_state);
			}

			flags_usSensorError.bits.echo = 1;

			//Informs to higher layer
			if(usSensorCallback != NULL) {
				usSensorCallback(__usSensorEvent_errorEcho, NULL);
			}

			fsmManager_gotoState(&usSensor_state,__usSensor_error);

			if(fsmManager_isStateOut(&usSensor_state)) {
				fsmManager_stateOut(&usSensor_state);
			}
			break;



		case __usSensor_errorEchoOverflow:
			if(fsmManager_isStateIn(&usSensor_state)) {
				fsmManager_stateIn(&usSensor_state);
			}

			flags_usSensorError.bits.echo = 1;

			//Informs to higher layer
			if(usSensorCallback != NULL) {
				usSensorCallback(__usSensorEvent_errorEcho, NULL);
			}

			fsmManager_gotoState(&usSensor_state,__usSensor_error);

			if(fsmManager_isStateOut(&usSensor_state)) {
				fsmManager_stateOut(&usSensor_state);
			}
			break;



		case __usSensor_error:
			if(fsmManager_isStateIn(&usSensor_state)) {
				fsmManager_stateIn(&usSensor_state);
			}

			fsmManager_gotoState(&usSensor_state,__usSensor_idle);

			if(fsmManager_isStateOut(&usSensor_state)) {
				fsmManager_stateOut(&usSensor_state);
			}
			break;



		default:
			if(fsmManager_isStateIn(&usSensor_state)) {
				fsmManager_stateIn(&usSensor_state);
			}

			fsmManager_gotoState(&usSensor_state,__usSensor_idle);

			if(fsmManager_isStateOut(&usSensor_state)) {
				fsmManager_stateOut(&usSensor_state);
			}
	}
}

void usSensor_powerOn(void)
{
	pinUsVdd_write(1);
}

void usSensor_powerOff(void)
{
	pinUsVdd_write(0);
}

void usSensor_measure(float temp)
{
	temperature = temp;

	flags_usSensor.bits.requestMeasure = 1;
}

uint8_t usSensor_isMeasuring(void)
{
	return flags_usSensor.bits.isMeasuring;
}





static float getSoundSpeed(void)
{
	float speed = 0;

	speed = numberFloat_getLinearValue(temperature, tableTempVsSpeed, sizeof(tableTempVsSpeed)/(sizeof(float)*2));

	return speed;
}

void usSensor_setCallback(void (*cb)(_usSensor_event, void*))
{
	usSensorCallback = cb;
}





//Pin Management
/*
 	PIN				IN/OUT			FUNCTION
 	--------------	--------------	--------------
	US_VDD			input			gpio
	US_TRIGGER		input			gpio
	US_ECHO			output			input capture
*/

static void pinUsVdd_write(uint8_t state)
{
	HAL_GPIO_WritePin(US_SENSOR_VDD_PORT, US_SENSOR_VDD_PIN, state);
}

static void pinUsTrigger_write(uint8_t state)
{
	HAL_GPIO_WritePin(US_SENSOR_TRIGGER_PORT, US_SENSOR_TRIGGER_PIN, state);
}

static void pinUsEcho_inputCaptureStart(void)
{
	HAL_TIM_IC_Start_IT(usHtim, US_SENSOR_ECHO_TIM_CHANNEL);
}

static void pinUsEcho_inputCaptureStop(void)
{
	HAL_TIM_IC_Stop_IT(usHtim, US_SENSOR_ECHO_TIM_CHANNEL);
}

void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
	if(htim->Instance == usHtim->Instance && htim->Channel == usHtim->Channel) {
		if(flags_usSensor.bits.isFirstCaptured == 0) {
			flags_usSensor.bits.isFirstCaptured = 1;

			icVal1 = HAL_TIM_ReadCapturedValue(usHtim, US_SENSOR_ECHO_TIM_CHANNEL);
			__HAL_TIM_SET_CAPTUREPOLARITY(usHtim, US_SENSOR_ECHO_TIM_CHANNEL, TIM_INPUTCHANNELPOLARITY_FALLING);
		} else {
			flags_usSensor.bits.isFirstCaptured = 0;

			//Take difference between read captures
			icVal2 = HAL_TIM_ReadCapturedValue(usHtim, US_SENSOR_ECHO_TIM_CHANNEL);
			__HAL_TIM_SET_COUNTER(usHtim, 0);
			if(icVal2 > icVal1) icDif = icVal2-icVal1;
			if(icVal1 > icVal2) icDif = (0xffff-icVal1)+icVal2;

			//Get echo time
			echoTime = icDif*mFactor;

			flags_usSensor.bits.receivedEcho = 1;
			__HAL_TIM_SET_CAPTUREPOLARITY(usHtim, US_SENSOR_ECHO_TIM_CHANNEL, TIM_INPUTCHANNELPOLARITY_RISING);
		}
	}
}
