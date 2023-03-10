/* INCLUDES */
#include "nh3SensorManager.h"
#include "fsmManager.h"
#include "softTimer.h"
#include "utilities.h"
#include <math.h>



/* DEFINES */
#define NH3_SENSOR_N_AVERAGES	10

/* TYPEDEF */
//FSM
typedef enum {
	__nh3Sensor_idle,
	__nh3Sensor_getAmmoniac
} _nh3Sensor_state;

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
} _flags_nh3Sensor;

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
} _flags_nh3SensorError;


/* VARIABLES */
//ADC
static ADC_HandleTypeDef *nh3Hadc;
static float nh3Ppm;
static float nh3Adc;
static float nh3Rs;		//Rs
static float nh3Ratio;	//Rs/Ro ratio
static float nh3Slope;
static float nh3Intersec;

//FSM
static fsm_t nh3Sensor_state;

//Flags
static _flags_nh3Sensor flags_nh3Sensor;
static _flags_nh3SensorError flags_nh3SensorError;
static float nh3SensorPpmAverage;
static uint32_t nAverages;

//Timers
static SoftTimer_t timer;



/* FUNCTION PROTOTYPES */
//Callback
static void (*nh3SensorCallback)(_nh3Sensor_event evt, void* payload);



static void ADC_Select_CHNh3(void);
static void nh3_adcStart(void);
static uint32_t nh3_adcGetValue(void);
static void nh3_adcStop(void);
static void pinNh3Vdd_write(uint8_t state);




void nh3Sensor_init(ADC_HandleTypeDef *hadc)
{
	//ADC
	nh3Hadc = hadc;

	//FSM
	fsmManager_init(&nh3Sensor_state, __nh3Sensor_idle);

	//Flags
	flags_nh3Sensor.dword = 0;
	flags_nh3SensorError.dword = 0;
}

void nh3Sensor_handler(void)
{
	switch(fsmManager_getState(&nh3Sensor_state)) {
		case __nh3Sensor_idle:
			if(fsmManager_isStateIn(&nh3Sensor_state)) {
				fsmManager_stateIn(&nh3Sensor_state);

				flags_nh3Sensor.bits.isMeasuring = 0;

				//nh3Ppm = 0;
			}

			if(flags_nh3Sensor.bits.requestMeasure) {
				flags_nh3Sensor.bits.isMeasuring = 1;

				fsmManager_gotoState(&nh3Sensor_state,__nh3Sensor_getAmmoniac);
			}


			if(fsmManager_isStateOut(&nh3Sensor_state)) {
				fsmManager_stateOut(&nh3Sensor_state);
			}
			break;



		case __nh3Sensor_getAmmoniac:
			if(fsmManager_isStateIn(&nh3Sensor_state)) {
				fsmManager_stateIn(&nh3Sensor_state);

				nh3_adcStart();

				nAverages = 0;
				nh3SensorPpmAverage = 0;

				softTimer_start(&timer, 50);
			}

			if(nAverages < NH3_SENSOR_N_AVERAGES) {
				if(softTimer_expired(&timer)) {
					softTimer_start(&timer, 50);

					nAverages++;

					nh3SensorPpmAverage += (float)nh3_adcGetValue()/NH3_SENSOR_N_AVERAGES;
				}
			}
			else {
				nh3Adc = (float) nh3SensorPpmAverage;							//Gets adc value
				nh3Rs = NH3_RL*(4095/nh3Adc-1);									//Gets Rs
				nh3Ratio = nh3Rs/NH3_R0;										//Gets Rs/Ro ratio
				nh3Slope = log10(NH3_P2_Y/NH3_P1_Y)/log10(NH3_P2_X/NH3_P1_X);	//Gets m
				nh3Intersec = log10(NH3_P1_Y)-nh3Slope*log10(NH3_P1_X);			//Gets b
				nh3Ppm = pow(10, (log10(nh3Ratio)-nh3Intersec)/nh3Slope);		//Gets ppm

				if(nh3SensorCallback != NULL) {
					nh3SensorCallback(__nh3SensorEvent_okMeasuring, (float *) &nh3Ppm);
				}

				fsmManager_gotoState(&nh3Sensor_state,__nh3Sensor_idle);
			}

			if(fsmManager_isStateOut(&nh3Sensor_state)) {
				fsmManager_stateOut(&nh3Sensor_state);

				nh3_adcStop();

				flags_nh3Sensor.bits.requestMeasure = 0;
			}
			break;
	}
}

void nh3Sensor_measure(void)
{
	flags_nh3Sensor.bits.requestMeasure = 1;
}

uint8_t nh3Sensor_isMeasuring(void)
{
	return flags_nh3Sensor.bits.isMeasuring;
}

void nh3Sensor_setCallback(void (*cb)(_nh3Sensor_event, void*))
{
	nh3SensorCallback = cb;
}

void nh3Sensor_powerOn(void)
{
	pinNh3Vdd_write(1);
}

void nh3Sensor_powerOff(void)
{
	pinNh3Vdd_write(0);
}



static void ADC_Select_CHNh3(void)
{
	ADC_ChannelConfTypeDef sConfig = {0};

	sConfig.Channel = ADC_CHANNEL_6;
	sConfig.Rank = 1;
	sConfig.SamplingTime = ADC_SAMPLETIME_7CYCLES_5;
	if(HAL_ADC_ConfigChannel(nh3Hadc, &sConfig) != HAL_OK) {
		Error_Handler();
	}
}

static void nh3_adcStart(void)
{
	ADC_Select_CHNh3();
	HAL_ADC_Start(nh3Hadc);
	HAL_ADC_PollForConversion(nh3Hadc, 100);
}

static uint32_t nh3_adcGetValue(void)
{
	return HAL_ADC_GetValue(nh3Hadc);
}

static void nh3_adcStop(void)
{
	HAL_ADC_Stop(nh3Hadc);
}

static void pinNh3Vdd_write(uint8_t state)
{
	HAL_GPIO_WritePin(NH3_SENSOR_VDD_PORT, NH3_SENSOR_VDD_PIN, state);
}
