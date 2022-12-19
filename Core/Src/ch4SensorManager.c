/* INCLUDES */
#include "ch4SensorManager.h"
#include "fsmManager.h"
#include "utilities.h"




/* TYPEDEF */
//FSM
typedef enum {
	__ch4Sensor_idle,
	__ch4Sensor_getAmmoniac
} _ch4Sensor_state;

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
} _flags_ch4Sensor;

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
} _flags_ch4SensorError;


/* VARIABLES */
//ADC
static ADC_HandleTypeDef *ch4Hadc;
static float ch4Ppm;

//FSM
static fsm_t ch4Sensor_state;

//Flags
static _flags_ch4Sensor flags_ch4Sensor;
static _flags_ch4SensorError flags_ch4SensorError;

/* FUNCTION PROTOTYPES */
//Callback
static void (*ch4SensorCallback)(_ch4Sensor_event evt, void* payload);



static void ADC_Select_CHCh4(void);
static void ch4_adcStart(void);
static uint32_t ch4_adcGetValue(void);
static void ch4_adcStop(void);
static void pinCh4Vdd_write(uint8_t state);




void ch4Sensor_init(ADC_HandleTypeDef *hadc)
{
	//ADC
	ch4Hadc = hadc;

	//FSM
	fsmManager_init(&ch4Sensor_state, __ch4Sensor_idle);

	//Flags
	flags_ch4Sensor.dword = 0;
	flags_ch4SensorError.dword = 0;
}

void ch4Sensor_handler(void)
{
	switch(fsmManager_getState(&ch4Sensor_state)) {
		case __ch4Sensor_idle:
			if(fsmManager_isStateIn(&ch4Sensor_state)) {
				fsmManager_stateIn(&ch4Sensor_state);

				flags_ch4Sensor.bits.isMeasuring = 0;

				//ch4Ppm = 0;
			}

			if(flags_ch4Sensor.bits.requestMeasure) {
				flags_ch4Sensor.bits.isMeasuring = 1;

				fsmManager_gotoState(&ch4Sensor_state,__ch4Sensor_getAmmoniac);
			}


			if(fsmManager_isStateOut(&ch4Sensor_state)) {
				fsmManager_stateOut(&ch4Sensor_state);
			}
			break;



		case __ch4Sensor_getAmmoniac:
			if(fsmManager_isStateIn(&ch4Sensor_state)) {
				fsmManager_stateIn(&ch4Sensor_state);

				ch4_adcStart();
			}

			ch4Ppm = (float)ch4_adcGetValue();
			if(ch4SensorCallback != NULL) {
				ch4SensorCallback(__ch4SensorEvent_okMeasuring, (float *) &ch4Ppm);
			}

			fsmManager_gotoState(&ch4Sensor_state,__ch4Sensor_idle);

			if(fsmManager_isStateOut(&ch4Sensor_state)) {
				fsmManager_stateOut(&ch4Sensor_state);

				ch4_adcStop();

				flags_ch4Sensor.bits.requestMeasure = 0;
			}
			break;
	}
}

void ch4Sensor_measure(void)
{
	flags_ch4Sensor.bits.requestMeasure = 1;
}

uint8_t ch4Sensor_isMeasuring(void)
{
	return flags_ch4Sensor.bits.isMeasuring;
}

void ch4Sensor_setCallback(void (*cb)(_ch4Sensor_event, void*))
{
	ch4SensorCallback = cb;
}

void ch4Sensor_powerOn(void)
{
	pinCh4Vdd_write(1);
}

void ch4Sensor_powerOff(void)
{
	pinCh4Vdd_write(0);
}



static void ADC_Select_CHCh4(void)
{
	ADC_ChannelConfTypeDef sConfig = {0};

	sConfig.Channel = ADC_CHANNEL_4;
	sConfig.Rank = 1;
	sConfig.SamplingTime = ADC_SAMPLETIME_7CYCLES_5;
	if(HAL_ADC_ConfigChannel(ch4Hadc, &sConfig) != HAL_OK) {
		Error_Handler();
	}
}

static void ch4_adcStart(void)
{
	ADC_Select_CHCh4();
	HAL_ADC_Start(ch4Hadc);
	HAL_ADC_PollForConversion(ch4Hadc, 100);
}

static uint32_t ch4_adcGetValue(void)
{
	return HAL_ADC_GetValue(ch4Hadc);
}

static void ch4_adcStop(void)
{
	HAL_ADC_Stop(ch4Hadc);
}

static void pinCh4Vdd_write(uint8_t state)
{
	//HAL_GPIO_WritePin(, , state);
}

