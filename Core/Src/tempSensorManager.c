/* INCLUDES */
#include "tempSensorManager.h"
#include "fsmManager.h"
#include "utilities.h"



/* DEFINES */
#define V_25		0.76
#define AVG_SLOPE	0.025



/* TYPEDEF */
//FSM
typedef enum {
	__tempSensor_idle,
	__tempSensor_getTemperature
} _tempSensor_state;

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
		uint8_t bit6:1;
		uint8_t bit7:1;
		uint8_t bit8:1;
		uint8_t bit9:1;
		uint8_t bit10:1;
		uint8_t isMeasuring:1;
	} bits;
} _flags_tempSensor;

typedef union {
	uint32_t dword;
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
	} bits;
} _flags_tempSensorError;


/* VARIABLES */
//ADC
static ADC_HandleTypeDef *tempHadc;
static float tCelcius;

//FSM
static fsm_t tempSensor_state;

//Flags
static _flags_tempSensor flags_tempSensor;
static _flags_tempSensorError flags_tempSensorError;


/* FUNCTION PROTOTYPES */
//Callback
static void (*tempSensorCallback)(_tempSensor_event evt, void* payload);



static void ADC_Select_CHTemp(void);
static void temp_adcStart(void);
static uint32_t temp_adcGetValue(void);
static void temp_adcStop(void);




void tempSensor_init(ADC_HandleTypeDef *hadc)
{
	//ADC
	tempHadc = hadc;

	//FSM
	fsmManager_init(&tempSensor_state, __tempSensor_idle);

	//Flags
	flags_tempSensor.dword = 0;
	flags_tempSensorError.dword = 0;
}

void tempSensor_handler(void)
{
	switch(fsmManager_getState(&tempSensor_state)) {
		case __tempSensor_idle:
			if(fsmManager_isStateIn(&tempSensor_state)) {
				fsmManager_stateIn(&tempSensor_state);

				flags_tempSensor.bits.isMeasuring = 0;

				//tCelcius = 0;
			}

			if(flags_tempSensor.bits.requestMeasure) {
				flags_tempSensor.bits.isMeasuring = 1;

				fsmManager_gotoState(&tempSensor_state,__tempSensor_getTemperature);
			}


			if(fsmManager_isStateOut(&tempSensor_state)) {
				fsmManager_stateOut(&tempSensor_state);
			}
			break;



		case __tempSensor_getTemperature:
			if(fsmManager_isStateIn(&tempSensor_state)) {
				fsmManager_stateIn(&tempSensor_state);

				temp_adcStart();
			}

			tCelcius = (3.3*((float)temp_adcGetValue())/4095 - V_25)/AVG_SLOPE + 25;
			if(tempSensorCallback != NULL) {
				tempSensorCallback(__tempSensorEvent_okMeasuring, (float *) &tCelcius);
			}

			fsmManager_gotoState(&tempSensor_state,__tempSensor_idle);

			if(fsmManager_isStateOut(&tempSensor_state)) {
				fsmManager_stateOut(&tempSensor_state);

				temp_adcStop();

				flags_tempSensor.bits.requestMeasure = 0;
			}
			break;
	}
}

void tempSensor_measure(void)
{
	flags_tempSensor.bits.requestMeasure = 1;
}

uint8_t tempSensor_isMeasuring(void)
{
	return flags_tempSensor.bits.isMeasuring;
}

void tempSensor_setCallback(void (*cb)(_tempSensor_event, void*))
{
	tempSensorCallback = cb;
}





static void ADC_Select_CHTemp(void)
{
	ADC_ChannelConfTypeDef sConfig = {0};

	sConfig.Channel = ADC_CHANNEL_TEMPSENSOR;
	sConfig.Rank = 1;
	sConfig.SamplingTime = ADC_SAMPLETIME_41CYCLES_5;
	if(HAL_ADC_ConfigChannel(tempHadc, &sConfig) != HAL_OK) {
		Error_Handler();
	}
}

static void temp_adcStart(void)
{
	ADC_Select_CHTemp();
	HAL_ADC_Start(tempHadc);
	HAL_ADC_PollForConversion(tempHadc, 100);
}

static uint32_t temp_adcGetValue(void)
{
	return HAL_ADC_GetValue(tempHadc);
}

static void temp_adcStop(void)
{
	HAL_ADC_Stop(tempHadc);
}

