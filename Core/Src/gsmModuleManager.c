/* INCLUDES */
#include "gsmModuleManager.h"
#include "gsmModuleAtCommands.h"
#include "fsmManager.h"
#include "softTimer.h"
#include "utilities.h"



/* DEFINES */
#define GSMRXDATA_LENGTH		400
#define GSMRXDATACHUNK_LENGTH	200
#define COUNTGPSINFO_MAX		3

/* PRIVATE VARIABLES */
static UART_HandleTypeDef *gsmHuart;
static uint8_t gsmRxData[GSMRXDATA_LENGTH];				//Ring buffer
static uint8_t gsmRxDataChunk[GSMRXDATACHUNK_LENGTH];
static volatile uint32_t gsmRxPtrIn = 0;
static volatile uint32_t gsmRxPtrInPrev = 0;
static volatile uint32_t gsmRxPtrOut = 0;
static uint32_t gsmRxDataChunkLen;

static uint8_t countGpsInfo;

static uint8_t dataToSend[200];
static uint8_t gsmInfo[100];

typedef enum {
	__gsmModule_operator_unknown = 0,
	__gsmModule_operator_personal,
	__gsmModule_operator_claro,
	__gsmModule_operator_movistar
} _gsmModule_operator;
_gsmModule_operator gsmModule_operator;

/* FLAGS */
typedef union {
	uint32_t dword;
	struct {
		uint8_t requestPowerOn:1;
		uint8_t requestPowerOff:1;
		uint8_t requestGpsOn:1;
		uint8_t requestGpsInfo:1;
		uint8_t requestGpsOff:1;
		uint8_t requestServerConnection:1;
		uint8_t requestServerDataSend:1;
		uint8_t requestServerDataReceive:1;
		uint8_t requestServerDisconnection:1;

		uint8_t receivedRx:1;

		uint8_t isPowered:1;
		uint8_t isGpsOn:1;
		uint8_t isGpsFixed:1;			//GPS Location 2D or 3D Fix
		uint8_t isServerConnected:1;
		uint8_t isServerDataSent:1;
		uint8_t isServerDataReceived:1;

	} bits;
} _flags_gsmModule;

typedef union {
	uint32_t dword;
	struct {
		uint8_t powerOn:1;
		uint8_t powerOff:1;
		uint8_t gpsOn:1;
		uint8_t gpsInfo:1;
		uint8_t gpsOff:1;
		uint8_t serverConnection:1;
		uint8_t serverSendData:1;
		uint8_t serverDisconnection:1;
		uint8_t bit8:1;
	} bits;
} _flags_gsmModuleError;

typedef union {
	uint32_t dword;
	struct {
		uint8_t cpinNotReady:1;
		uint8_t cpinSimPin:1;
		uint8_t cpinReady:1;
		uint8_t creg0:1;
		uint8_t creg1:1;
		uint8_t creg2:1;
		uint8_t httpActionOk:1;
		uint8_t httpActionError:1;
		uint8_t bit8:1;
	} bits;
} _flags_gsmModuleUnsolicited;

static _flags_gsmModule flags_gsmModule;
static _flags_gsmModuleError flags_gsmModuleError;
static _flags_gsmModuleUnsolicited flags_gsmModuleUnsolicited;


/* TIMERS */
static SoftTimer_t timer;
static SoftTimer_t timeout;



/* FSM */
typedef enum {
	__gsmModule_requestPowerOn_idle,
	__gsmModule_requestPowerOn_pwrKeyOn,
	__gsmModule_requestPowerOn_pwrKeyOnWait,
	__gsmModule_requestPowerOn_pwrKeyOff,
	__gsmModule_requestPowerOn_send_at,
	__gsmModule_requestPowerOn_check_at,
	__gsmModule_requestPowerOn_error
} _gsmModule_requestPowerOn_state;

typedef enum {
	__gsmModule_requestPowerOff_idle,
	__gsmModule_requestPowerOff_send_atCpowd1,
	__gsmModule_requestPowerOff_check_atCpowd1,
	__gsmModule_requestPowerOff_send_at,
	__gsmModule_requestPowerOff_check_at,
	__gsmModule_requestPowerOff_error
} _gsmModule_requestPowerOff_state;

typedef enum {
	__gsmModule_requestGpsOn_idle,
	__gsmModule_requestGpsOn_send_atCgpspwr1,
	__gsmModule_requestGpsOn_waitOk_atCgpspwr1,
	__gsmModule_requestGpsOn_send_atCgpsrst0,
	__gsmModule_requestGpsOn_waitOk_atCgpsrst0,
	__gsmModule_requestGpsOn_error
} _gsmModule_requestGpsOn_state;

typedef enum {
	__gsmModule_requestGpsInfo_idle,
	__gsmModule_requestGpsInfo_send_atCgpsstatus,
	__gsmModule_requestGpsInfo_check_atCgpsstatus,
	__gsmModule_requestGpsInfo_send_atCgpsinf2,
	__gsmModule_requestGpsInfo_get_atCgpsinf2,
	__gsmModule_requestGpsInfo_error
} _gsmModule_requestGpsInfo_state;

typedef enum {
	__gsmModule_requestGpsOff_idle,
	__gsmModule_requestGpsOff_send_atCgpsrst0,
	__gsmModule_requestGpsOff_waitOk_atCgpsrst0,
	__gsmModule_requestGpsOff_send_atCgpsrst1,
	__gsmModule_requestGpsOff_waitOk_atCgpsrst1,
	__gsmModule_requestGpsOff_send_atCgpspwr0,
	__gsmModule_requestGpsOff_waitOk_atCgpspwr0,
	__gsmModule_requestGpsOff_error
} _gsmModule_requestGpsOff_state;

typedef enum {
	__gsmModule_requestServerConnection_idle,
	__gsmModule_requestServerConnection_send_atCmee1,
	__gsmModule_requestServerConnection_waitOk_atCmee1,
	__gsmModule_requestServerConnection_send_atCfun0,
	__gsmModule_requestServerConnection_waitOk_atCfun0,
	__gsmModule_requestServerConnection_send_atCfun1,
	__gsmModule_requestServerConnection_waitOk_atCfun1,
	__gsmModule_requestServerConnection_send_atCpin,
	__gsmModule_requestServerConnection_check_atCpin,
	__gsmModule_requestServerConnection_send_atCband,
	__gsmModule_requestServerConnection_waitOk_atCband,
	__gsmModule_requestServerConnection_send_atCreg1,
	__gsmModule_requestServerConnection_check_atCreg1,
	__gsmModule_requestServerConnection_waitOk_atCreg1,
	__gsmModule_requestServerConnection_send_atCreg,
	__gsmModule_requestServerConnection_check_atCreg,
	__gsmModule_requestServerConnection_send_atCops,
	__gsmModule_requestServerConnection_get_atCops,
	__gsmModule_requestServerConnection_send_atSapbr3_contype,
	__gsmModule_requestServerConnection_waitOk_atSapbr3_contype,
	__gsmModule_requestServerConnection_send_atSapbr3_apn,
	__gsmModule_requestServerConnection_waitOk_atSapbr3_apn,
	__gsmModule_requestServerConnection_send_atSapbr3_user,
	__gsmModule_requestServerConnection_waitOk_atSapbr3_user,
	__gsmModule_requestServerConnection_send_atSapbr3_pwd,
	__gsmModule_requestServerConnection_waitOk_atSapbr3_pwd,
	__gsmModule_requestServerConnection_send_atSapbr1,
	__gsmModule_requestServerConnection_waitOk_atSapbr1,
	__gsmModule_requestServerConnection_send_atSapbr2,
	__gsmModule_requestServerConnection_check_atSapbr2,
	__gsmModule_requestServerConnection_send_atHttpinit,
	__gsmModule_requestServerConnection_waitOk_atHttpinit,
	__gsmModule_requestServerConnection_send_atHttppara_cid,
	__gsmModule_requestServerConnection_waitOk_atHttppara_cid,

	__gsmModule_requestServerConnection_error
} _gsmModule_requestServerConnection_state;


typedef enum {
	__gsmModule_requestServerDataSend_idle,
	__gsmModule_requestServerDataSend_send_atHttppara_url,
	__gsmModule_requestServerDataSend_waitOk_atHttppara_url,
	__gsmModule_requestServerDataSend_send_atHttpaction1,
	__gsmModule_requestServerDataSend_waitOk_atHttpaction1,
	__gsmModule_requestServerDataSend_error
} _gsmModule_requestServerDataSend_state;

typedef enum {
	__gsmModule_requestServerDataReceive_idle,

	__gsmModule_requestServerDataReceive_error
} _gsmModule_requestServerDataReceive_state;

typedef enum {
	__gsmModule_requestServerDisconnection_idle,
	__gsmModule_requestServerDisconnection_send_atHttpterm,
	__gsmModule_requestServerDisconnection_waitOk_atHttpterm,
	__gsmModule_requestServerDisconnection_send_atSapbr0,
	__gsmModule_requestServerDisconnection_waitOk_atSapbr0,
	__gsmModule_requestServerDisconnection_send_atSapbr2,
	__gsmModule_requestServerDisconnection_check_atSapbr2,
	__gsmModule_requestServerDisconnection_error
} _gsmModule_requestServerDisconnection_state;


static fsm_t gsmModule_requestPowerOn_state;
static fsm_t gsmModule_requestPowerOff_state;
static fsm_t gsmModule_requestGpsOn_state;
static fsm_t gsmModule_requestGpsInfo_state;
static fsm_t gsmModule_requestGpsOff_state;
static fsm_t gsmModule_requestServerConnection_state;
static fsm_t gsmModule_requestServerDataSend_state;
static fsm_t gsmModule_requestServerDataReceive_state;
static fsm_t gsmModule_requestServerDisconnection_state;



/* PRIVATE FUNCTIONS */
static void handle_unsolicitedMessages(void);
static void handle_requestPowerOn(void);
static void handle_requestPowerOff(void);
static void handle_requestGpsOn(void);
static void handle_requestGpsInfo(void);
static void handle_requestGpsOff(void);
static void handle_requestServerConnection(void);
static void handle_requestServerDataSend(void);
static void handle_requestServerDisconnection(void);
//Pin management
static void pinGsmVdd_write(uint8_t value);
static void pinGsmPwrkey_write(uint8_t value);
static void pinGsmUartTx_transmit(uint8_t *gsmModule_command);
static void pinGsmUartRx_receive(void);
//Callback
static void (*gsmModuleCallback)(_gsmModule_event evt, void* payload);



void gsmModule_init(UART_HandleTypeDef *huart)
{
	//FSM
	fsmManager_init(&gsmModule_requestPowerOn_state, __gsmModule_requestPowerOn_idle);
	fsmManager_init(&gsmModule_requestPowerOff_state, __gsmModule_requestPowerOff_idle);
	fsmManager_init(&gsmModule_requestGpsOn_state, __gsmModule_requestGpsOn_idle);
	fsmManager_init(&gsmModule_requestGpsInfo_state, __gsmModule_requestGpsInfo_idle);
	fsmManager_init(&gsmModule_requestGpsOff_state, __gsmModule_requestGpsOff_idle);
	fsmManager_init(&gsmModule_requestServerConnection_state, __gsmModule_requestServerConnection_idle);
	fsmManager_init(&gsmModule_requestServerDataSend_state, __gsmModule_requestServerDataSend_idle);
	fsmManager_init(&gsmModule_requestServerDataReceive_state, __gsmModule_requestServerDataReceive_idle);
	fsmManager_init(&gsmModule_requestServerDisconnection_state, __gsmModule_requestServerDisconnection_idle);

	//UART
	gsmHuart = huart;
	//pinGsmUartRx_receive();

	//Flags
	flags_gsmModule.dword = 0;
	flags_gsmModuleError.dword = 0;
	flags_gsmModuleUnsolicited.dword = 0;

	//Pin
	pinGsmVdd_write(0);
	pinGsmPwrkey_write(0);

	//Variables
	gsmModule_operator = __gsmModule_operator_unknown;
}

void gsmModule_setCallback(void (*cb)(_gsmModule_event, void*))
{
	gsmModuleCallback = cb;
}



void gsmModule_powerOn(void)
{
	pinGsmVdd_write(1);
}


void gsmModule_powerOff(void)
{
	pinGsmVdd_write(0);
}

void gsmModule_pwrkeyOn(void)
{
	flags_gsmModule.bits.requestPowerOn = 1;
}

void gsmModule_pwrkeyOff(void)
{
	flags_gsmModule.bits.requestPowerOff = 1;
}

void gsmModule_gpsOn(void)
{
	flags_gsmModule.bits.requestGpsOn = 1;
}

void gsmModule_gpsInfo(uint8_t enable)
{
	flags_gsmModule.bits.requestGpsInfo = (enable ? 1 : 0);
}

void gsmModule_gpsOff(void)
{
	flags_gsmModule.bits.requestGpsOff = 1;
}

void gsmModule_serverConnect(void)
{
	flags_gsmModule.bits.requestServerConnection = 1;
}

void gsmModule_serverDataSend(uint8_t *data)
{
	string_appendString(dataToSend, (uint8_t *) gsmModule_command_httppara_url);
	string_appendString(dataToSend, data);
	string_appendString(dataToSend, (uint8_t *) "\r\n");

	flags_gsmModule.bits.requestServerDataSend = 1;
}

void gsmModule_serverDataReceive(void)
{
	flags_gsmModule.bits.requestServerDataReceive = 1;
}

void gsmModule_serverDisconnect(void)
{
	flags_gsmModule.bits.requestServerDisconnection = 1;
}

uint8_t gsmModule_isPowered(void)
{
	return flags_gsmModule.bits.isPowered;
}

uint8_t gsmModule_isGpsOn(void)
{
	return flags_gsmModule.bits.isGpsOn;
}

uint8_t gsmModule_isGpsFixed(void)
{
	return flags_gsmModule.bits.isGpsFixed;
}

uint8_t gsmModule_isServerConnected(void)
{
	return flags_gsmModule.bits.isServerConnected;
}

uint8_t gsmModule_isServerDataSent(void)
{
	return flags_gsmModule.bits.isServerDataSent;
}

uint8_t gsmModule_isServerDataReceived(void)
{
	return flags_gsmModule.bits.isServerDataReceived;
}





uint8_t gsmModule_requestedPowerOn(void)
{
	return flags_gsmModule.bits.requestPowerOn;
}

uint8_t gsmModule_requestedPowerOff(void)
{
	return flags_gsmModule.bits.requestPowerOff;
}

uint8_t gsmModule_requestedGpsOn(void)
{
	return flags_gsmModule.bits.requestGpsOn;
}

uint8_t gsmModule_requestedGpsInfo(void)
{
	return flags_gsmModule.bits.requestGpsInfo;
}

uint8_t gsmModule_requestedGpsOff(void)
{
	return flags_gsmModule.bits.requestGpsOff;
}

uint8_t gsmModule_requestedServerConnection(void)
{
	return flags_gsmModule.bits.requestServerConnection;
}

uint8_t gsmModule_requestedServerDataSend(void)
{
	return flags_gsmModule.bits.requestServerDataSend;
}

uint8_t gsmModule_requestedServerDataReceive(void)
{
	return flags_gsmModule.bits.requestServerDataReceive;
}

uint8_t gsmModule_requestedServerDisconnection(void)
{
	return flags_gsmModule.bits.requestServerDisconnection;
}



uint8_t gsmModule_errorServerSendData(void)
{
	return flags_gsmModuleError.bits.serverSendData;
}

static uint32_t contChunks;

void gsmModule_handler(void)
{
	if(gsmRxPtrInPrev != gsmRxPtrIn) {
		gsmRxPtrInPrev++;
		gsmRxPtrInPrev%=GSMRXDATA_LENGTH;

		gsmRxDataChunkLen = ringBufferOfUint8_popChunk(gsmRxData, gsmRxPtrOut, gsmRxPtrIn, '\n', GSMRXDATA_LENGTH, gsmRxDataChunk);
		if(gsmRxDataChunkLen > 0) {
			gsmRxPtrOut+=gsmRxDataChunkLen;
			gsmRxPtrOut%=GSMRXDATA_LENGTH;

			contChunks++;
		}
	}

	handle_unsolicitedMessages();

	if(flags_gsmModule.bits.requestPowerOn) {
		handle_requestPowerOn();
	}
	else if(flags_gsmModule.bits.requestPowerOff) {
		handle_requestPowerOff();
	}
	else {
		if(flags_gsmModule.bits.requestGpsOn) {
			handle_requestGpsOn();
		}
		else if(flags_gsmModule.bits.requestGpsInfo) {
			handle_requestGpsInfo();
		}
		else if(flags_gsmModule.bits.requestGpsOff) {
			handle_requestGpsOff();
		}
		else if(flags_gsmModule.bits.requestServerConnection) {
			handle_requestServerConnection();
		}
		else if(flags_gsmModule.bits.requestServerDataSend) {
			handle_requestServerDataSend();
		}
		else if(flags_gsmModule.bits.requestServerDataReceive) {
			//handle_requestServerDataReceive();
		}
		else if(flags_gsmModule.bits.requestServerDisconnection) {
			handle_requestServerDisconnection();
		}
	}

	gsmRxDataChunkLen = 0;
}

static void handle_unsolicitedMessages(void)
{
	if(string_containsWithinLength(gsmRxDataChunk, (uint8_t *) gsmModule_unsolicited_creg, gsmRxDataChunkLen)) {
		if(string_containsWithinLength(gsmRxDataChunk, (uint8_t *) gsmModule_unsolicited_creg0, gsmRxDataChunkLen)) {
			flags_gsmModuleUnsolicited.bits.creg0 = 1;
		}
		else if(string_containsWithinLength(gsmRxDataChunk, (uint8_t *) gsmModule_unsolicited_creg1, gsmRxDataChunkLen)) {
			flags_gsmModuleUnsolicited.bits.creg1 = 1;
		}
		else if(string_containsWithinLength(gsmRxDataChunk, (uint8_t *) gsmModule_unsolicited_creg2, gsmRxDataChunkLen)) {
			flags_gsmModuleUnsolicited.bits.creg2 = 1;
		}
	}
	else if(string_containsWithinLength(gsmRxDataChunk, (uint8_t *) gsmModule_response_httpaction, gsmRxDataChunkLen)) {
		if(string_containsWithinLength(gsmRxDataChunk, (uint8_t *) gsmModule_response_httpaction1_200, gsmRxDataChunkLen)) {
			flags_gsmModuleUnsolicited.bits.httpActionOk = 1;
		}
		else {
			flags_gsmModuleUnsolicited.bits.httpActionError = 1;
		}
	}
}

static void handle_requestPowerOn(void)
{
	switch(fsmManager_getState(&gsmModule_requestPowerOn_state)) {
		case __gsmModule_requestPowerOn_idle:
			if(fsmManager_isStateIn(&gsmModule_requestPowerOn_state)) {
				fsmManager_stateIn(&gsmModule_requestPowerOn_state);
			}

			fsmManager_gotoState(&gsmModule_requestPowerOn_state, __gsmModule_requestPowerOn_pwrKeyOn);

			if(fsmManager_isStateOut(&gsmModule_requestPowerOn_state)) {
				fsmManager_stateOut(&gsmModule_requestPowerOn_state);
			}
			break;



		case __gsmModule_requestPowerOn_pwrKeyOn:
			if(fsmManager_isStateIn(&gsmModule_requestPowerOn_state)) {
				fsmManager_stateIn(&gsmModule_requestPowerOn_state);
			}

			pinGsmPwrkey_write(1);
			fsmManager_gotoState(&gsmModule_requestPowerOn_state, __gsmModule_requestPowerOn_pwrKeyOnWait);

			if(fsmManager_isStateOut(&gsmModule_requestPowerOn_state)) {
				fsmManager_stateOut(&gsmModule_requestPowerOn_state);
			}
			break;



		case __gsmModule_requestPowerOn_pwrKeyOnWait:
			if(fsmManager_isStateIn(&gsmModule_requestPowerOn_state)) {
				fsmManager_stateIn(&gsmModule_requestPowerOn_state);

				softTimer_start(&timer, 1500);
			}

			if(softTimer_expired(&timer)) {
				fsmManager_gotoState(&gsmModule_requestPowerOn_state, __gsmModule_requestPowerOn_pwrKeyOff);
			}

			if(fsmManager_isStateOut(&gsmModule_requestPowerOn_state)) {
				fsmManager_stateOut(&gsmModule_requestPowerOn_state);
			}
			break;



		case __gsmModule_requestPowerOn_pwrKeyOff:
			if(fsmManager_isStateIn(&gsmModule_requestPowerOn_state)) {
				fsmManager_stateIn(&gsmModule_requestPowerOn_state);
			}

			pinGsmPwrkey_write(0);
			fsmManager_gotoState(&gsmModule_requestPowerOn_state, __gsmModule_requestPowerOn_send_at);

			if(fsmManager_isStateOut(&gsmModule_requestPowerOn_state)) {
				fsmManager_stateOut(&gsmModule_requestPowerOn_state);
			}
			break;



		case __gsmModule_requestPowerOn_send_at:
			if(fsmManager_isStateIn(&gsmModule_requestPowerOn_state)) {
				fsmManager_stateIn(&gsmModule_requestPowerOn_state);

				softTimer_start(&timeout, 4000);
			}

			if(softTimer_expired(&timeout)) {
				pinGsmUartTx_transmit((uint8_t *) gsmModule_command_at);

				fsmManager_gotoState(&gsmModule_requestPowerOn_state, __gsmModule_requestPowerOn_check_at);
			}

			if(fsmManager_isStateOut(&gsmModule_requestPowerOn_state)) {
				fsmManager_stateOut(&gsmModule_requestPowerOn_state);
			}
			break;



		case __gsmModule_requestPowerOn_check_at:
			if(fsmManager_isStateIn(&gsmModule_requestPowerOn_state)) {
				fsmManager_stateIn(&gsmModule_requestPowerOn_state);

				pinGsmUartRx_receive();
				softTimer_start(&timeout, 100);
			}


			if(string_containsWithinLength(gsmRxDataChunk, (uint8_t *) gsmModule_response_ok, gsmRxDataChunkLen)) {
				flags_gsmModule.bits.isPowered = 1;
				flags_gsmModule.bits.requestPowerOn = 0;

				fsmManager_gotoState(&gsmModule_requestPowerOn_state, __gsmModule_requestPowerOn_idle);
			}
			else if(softTimer_expired(&timeout)) {
				//The module is powered off
				fsmManager_gotoState(&gsmModule_requestPowerOn_state, __gsmModule_requestPowerOn_idle);
			}

			if(fsmManager_isStateOut(&gsmModule_requestPowerOn_state)) {
				fsmManager_stateOut(&gsmModule_requestPowerOn_state);
			}
			break;



		case __gsmModule_requestPowerOn_error:
			if(fsmManager_isStateIn(&gsmModule_requestPowerOn_state)) {
				fsmManager_stateIn(&gsmModule_requestPowerOn_state);
			}

			flags_gsmModule.bits.requestPowerOn = 0;
			flags_gsmModuleError.bits.powerOn = 1;
			fsmManager_gotoState(&gsmModule_requestPowerOn_state, __gsmModule_requestPowerOn_idle);

			if(fsmManager_isStateOut(&gsmModule_requestPowerOn_state)) {
				fsmManager_stateOut(&gsmModule_requestPowerOn_state);
			}
			break;



		default:
			if(fsmManager_isStateIn(&gsmModule_requestPowerOn_state)) {
				fsmManager_stateIn(&gsmModule_requestPowerOn_state);
			}

			flags_gsmModule.bits.requestPowerOn = 0;
			fsmManager_gotoState(&gsmModule_requestPowerOn_state, __gsmModule_requestPowerOn_idle);

			if(fsmManager_isStateOut(&gsmModule_requestPowerOn_state)) {
				fsmManager_stateOut(&gsmModule_requestPowerOn_state);
			}
			break;
	}
}

static void handle_requestPowerOff(void)
{
	switch(fsmManager_getState(&gsmModule_requestPowerOff_state)) {
		case __gsmModule_requestPowerOff_idle:
			if(fsmManager_isStateIn(&gsmModule_requestPowerOff_state)) {
				fsmManager_stateIn(&gsmModule_requestPowerOff_state);
			}

			fsmManager_gotoState(&gsmModule_requestPowerOff_state, __gsmModule_requestPowerOff_send_atCpowd1);

			if(fsmManager_isStateOut(&gsmModule_requestPowerOff_state)) {
				fsmManager_stateOut(&gsmModule_requestPowerOff_state);
			}
			break;



		case __gsmModule_requestPowerOff_send_atCpowd1:
			if(fsmManager_isStateIn(&gsmModule_requestPowerOff_state)) {
				fsmManager_stateIn(&gsmModule_requestPowerOff_state);
			}

			pinGsmUartTx_transmit((uint8_t *) gsmModule_command_cpowd1);
			fsmManager_gotoState(&gsmModule_requestPowerOff_state, __gsmModule_requestPowerOff_check_atCpowd1);

			if(fsmManager_isStateOut(&gsmModule_requestPowerOff_state)) {
				fsmManager_stateOut(&gsmModule_requestPowerOff_state);
			}
			break;



		case __gsmModule_requestPowerOff_check_atCpowd1:
			if(fsmManager_isStateIn(&gsmModule_requestPowerOff_state)) {
				fsmManager_stateIn(&gsmModule_requestPowerOff_state);

				softTimer_start(&timeout, 2000);
			}

			if(string_containsWithinLength(gsmRxDataChunk, (uint8_t *) gsmModule_response_normal_powerdown, gsmRxDataChunkLen)) {
				fsmManager_gotoState(&gsmModule_requestPowerOff_state, __gsmModule_requestPowerOff_send_at);
			}
			else if(softTimer_expired(&timeout)) {
				//The module is powered off
				fsmManager_gotoState(&gsmModule_requestPowerOn_state, __gsmModule_requestPowerOff_send_at);
			}

			if(fsmManager_isStateOut(&gsmModule_requestPowerOff_state)) {
				fsmManager_stateOut(&gsmModule_requestPowerOff_state);
			}
			break;



		case __gsmModule_requestPowerOff_send_at:
			if(fsmManager_isStateIn(&gsmModule_requestPowerOff_state)) {
				fsmManager_stateIn(&gsmModule_requestPowerOff_state);

				softTimer_start(&timeout, 2000);
			}

			if(softTimer_expired(&timeout)) {
				pinGsmUartTx_transmit((uint8_t *) gsmModule_command_at);

				fsmManager_gotoState(&gsmModule_requestPowerOff_state, __gsmModule_requestPowerOff_check_at);
			}

			if(fsmManager_isStateOut(&gsmModule_requestPowerOff_state)) {
				fsmManager_stateOut(&gsmModule_requestPowerOff_state);
			}
			break;



		case __gsmModule_requestPowerOff_check_at:
			if(fsmManager_isStateIn(&gsmModule_requestPowerOff_state)) {
				fsmManager_stateIn(&gsmModule_requestPowerOff_state);

				softTimer_start(&timeout, 500);
			}

			if(string_containsWithinLength(gsmRxDataChunk, (uint8_t *) gsmModule_response_ok, gsmRxDataChunkLen)) {
				fsmManager_gotoState(&gsmModule_requestPowerOff_state, __gsmModule_requestPowerOff_send_atCpowd1);
			}
			else if(softTimer_expired(&timeout)) {
				flags_gsmModule.bits.requestPowerOff = 0;
				flags_gsmModule.bits.isPowered = 0;

				fsmManager_gotoState(&gsmModule_requestPowerOff_state, __gsmModule_requestPowerOff_idle);
			}

			if(fsmManager_isStateOut(&gsmModule_requestPowerOff_state)) {
				fsmManager_stateOut(&gsmModule_requestPowerOff_state);
			}
			break;



		case __gsmModule_requestPowerOff_error:
			if(fsmManager_isStateIn(&gsmModule_requestPowerOff_state)) {
				fsmManager_stateIn(&gsmModule_requestPowerOff_state);
			}

			flags_gsmModuleError.bits.powerOff = 1;
			flags_gsmModule.bits.requestPowerOff = 0;

			fsmManager_gotoState(&gsmModule_requestPowerOff_state, __gsmModule_requestPowerOff_idle);

			if(fsmManager_isStateOut(&gsmModule_requestPowerOff_state)) {
				fsmManager_stateOut(&gsmModule_requestPowerOff_state);
			}
			break;
	}


}

static void handle_requestGpsOn(void)
{
	switch(fsmManager_getState(&gsmModule_requestGpsOn_state)) {
		case __gsmModule_requestGpsOn_idle:
			if(fsmManager_isStateIn(&gsmModule_requestGpsOn_state)) {
				fsmManager_stateIn(&gsmModule_requestGpsOn_state);
			}

			fsmManager_gotoState(&gsmModule_requestGpsOn_state, __gsmModule_requestGpsOn_send_atCgpspwr1);

			if(fsmManager_isStateOut(&gsmModule_requestGpsOn_state)) {
				fsmManager_stateOut(&gsmModule_requestGpsOn_state);
			}
			break;



		case __gsmModule_requestGpsOn_send_atCgpspwr1:
			if(fsmManager_isStateIn(&gsmModule_requestGpsOn_state)) {
				fsmManager_stateIn(&gsmModule_requestGpsOn_state);

				softTimer_start(&timeout, 100);
			}

			if(softTimer_expired(&timeout)) {
				pinGsmUartTx_transmit((uint8_t *) gsmModule_command_cgpspwr1);
				fsmManager_gotoState(&gsmModule_requestGpsOn_state, __gsmModule_requestGpsOn_waitOk_atCgpspwr1);
			}

			if(fsmManager_isStateOut(&gsmModule_requestGpsOn_state)) {
				fsmManager_stateOut(&gsmModule_requestGpsOn_state);
			}
			break;



		case __gsmModule_requestGpsOn_waitOk_atCgpspwr1:
			if(fsmManager_isStateIn(&gsmModule_requestGpsOn_state)) {
				fsmManager_stateIn(&gsmModule_requestGpsOn_state);

				softTimer_start(&timeout, 100);
			}

			if(string_containsWithinLength(gsmRxDataChunk, (uint8_t *) gsmModule_response_ok, gsmRxDataChunkLen)) {
				fsmManager_gotoState(&gsmModule_requestGpsOn_state, __gsmModule_requestGpsOn_send_atCgpsrst0);
			}
			else if(softTimer_expired(&timeout)) {
				fsmManager_gotoState(&gsmModule_requestGpsOn_state, __gsmModule_requestGpsOn_send_atCgpspwr1);
			}

			if(fsmManager_isStateOut(&gsmModule_requestGpsOn_state)) {
				fsmManager_stateOut(&gsmModule_requestGpsOn_state);
			}
			break;



		case __gsmModule_requestGpsOn_send_atCgpsrst0:
			if(fsmManager_isStateIn(&gsmModule_requestGpsOn_state)) {
				fsmManager_stateIn(&gsmModule_requestGpsOn_state);

				softTimer_start(&timeout, 100);
			}

			if(softTimer_expired(&timeout)) {
				pinGsmUartTx_transmit((uint8_t *) gsmModule_command_cgpsrst0);
				fsmManager_gotoState(&gsmModule_requestGpsOn_state, __gsmModule_requestGpsOn_waitOk_atCgpsrst0);
			}

			if(fsmManager_isStateOut(&gsmModule_requestGpsOn_state)) {
				fsmManager_stateOut(&gsmModule_requestGpsOn_state);
			}
			break;



		case __gsmModule_requestGpsOn_waitOk_atCgpsrst0:
			if(fsmManager_isStateIn(&gsmModule_requestGpsOn_state)) {
				fsmManager_stateIn(&gsmModule_requestGpsOn_state);

				softTimer_start(&timeout, 3000);
			}

			if(string_containsWithinLength(gsmRxDataChunk, (uint8_t *) gsmModule_response_ok, gsmRxDataChunkLen)) {
				flags_gsmModule.bits.isGpsOn = 1;
				flags_gsmModule.bits.requestGpsOn = 0;

				fsmManager_gotoState(&gsmModule_requestGpsOn_state, __gsmModule_requestGpsOn_idle);
			}
			else if(softTimer_expired(&timeout)) {
				fsmManager_gotoState(&gsmModule_requestGpsOn_state, __gsmModule_requestGpsOn_send_atCgpsrst0);
			}

			if(fsmManager_isStateOut(&gsmModule_requestGpsOn_state)) {
				fsmManager_stateOut(&gsmModule_requestGpsOn_state);
			}
			break;

		case __gsmModule_requestGpsOn_error:
			if(fsmManager_isStateIn(&gsmModule_requestGpsOn_state)) {
				fsmManager_stateIn(&gsmModule_requestGpsOn_state);
			}

			flags_gsmModuleError.bits.gpsOn = 1;
			flags_gsmModule.bits.requestGpsOn = 0;

			fsmManager_gotoState(&gsmModule_requestGpsOn_state, __gsmModule_requestGpsOn_idle);

			if(fsmManager_isStateOut(&gsmModule_requestGpsOn_state)) {
				fsmManager_stateOut(&gsmModule_requestGpsOn_state);
			}
			break;



		default:
			if(fsmManager_isStateIn(&gsmModule_requestGpsOn_state)) {
				fsmManager_stateIn(&gsmModule_requestGpsOn_state);
			}

			flags_gsmModule.bits.requestGpsOn = 0;

			fsmManager_gotoState(&gsmModule_requestGpsOn_state, __gsmModule_requestGpsOn_idle);

			if(fsmManager_isStateOut(&gsmModule_requestGpsOn_state)) {
				fsmManager_stateOut(&gsmModule_requestGpsOn_state);
			}
			break;
	}
}

static void handle_requestGpsInfo(void)
{
	switch(fsmManager_getState(&gsmModule_requestGpsInfo_state)) {
		case __gsmModule_requestGpsInfo_idle:
			if(fsmManager_isStateIn(&gsmModule_requestGpsInfo_state)) {
				fsmManager_stateIn(&gsmModule_requestGpsInfo_state);
			}

			fsmManager_gotoState(&gsmModule_requestGpsInfo_state, __gsmModule_requestGpsInfo_send_atCgpsstatus);

			if(fsmManager_isStateOut(&gsmModule_requestGpsInfo_state)) {
				fsmManager_stateOut(&gsmModule_requestGpsInfo_state);
			}
			break;



		case __gsmModule_requestGpsInfo_send_atCgpsstatus:
			if(fsmManager_isStateIn(&gsmModule_requestGpsInfo_state)) {
				fsmManager_stateIn(&gsmModule_requestGpsInfo_state);

				softTimer_start(&timeout, 10000);
			}

			if(softTimer_expired(&timeout)) {
				pinGsmUartTx_transmit((uint8_t *) gsmModule_command_cgpsstatus);
				fsmManager_gotoState(&gsmModule_requestGpsInfo_state, __gsmModule_requestGpsInfo_check_atCgpsstatus);
			}

			if(fsmManager_isStateOut(&gsmModule_requestGpsInfo_state)) {
				fsmManager_stateOut(&gsmModule_requestGpsInfo_state);
			}
			break;



		case __gsmModule_requestGpsInfo_check_atCgpsstatus:
			if(fsmManager_isStateIn(&gsmModule_requestGpsInfo_state)) {
				fsmManager_stateIn(&gsmModule_requestGpsInfo_state);

				softTimer_start(&timeout, 1000);
			}


			if(string_containsWithinLength(gsmRxDataChunk, (uint8_t *) gsmModule_response_gps2dFix, gsmRxDataChunkLen) ||
			   string_containsWithinLength(gsmRxDataChunk, (uint8_t *) gsmModule_response_gps3dFix, gsmRxDataChunkLen)) {

				fsmManager_gotoState(&gsmModule_requestGpsInfo_state, __gsmModule_requestGpsInfo_send_atCgpsinf2);
			}
			else if(softTimer_expired(&timeout)) {
				fsmManager_gotoState(&gsmModule_requestGpsInfo_state, __gsmModule_requestGpsInfo_send_atCgpsstatus);
			}

			if(fsmManager_isStateOut(&gsmModule_requestGpsInfo_state)) {
				fsmManager_stateOut(&gsmModule_requestGpsInfo_state);
			}
			break;



		case __gsmModule_requestGpsInfo_send_atCgpsinf2:
			if(fsmManager_isStateIn(&gsmModule_requestGpsInfo_state)) {
				fsmManager_stateIn(&gsmModule_requestGpsInfo_state);

				softTimer_start(&timeout, 8000);
			}

			if(softTimer_expired(&timeout)) {
				pinGsmUartTx_transmit((uint8_t *) gsmModule_command_cgpsinf2);
				fsmManager_gotoState(&gsmModule_requestGpsInfo_state, __gsmModule_requestGpsInfo_get_atCgpsinf2);

			}

			if(fsmManager_isStateOut(&gsmModule_requestGpsInfo_state)) {
				fsmManager_stateOut(&gsmModule_requestGpsInfo_state);
			}
			break;



		case __gsmModule_requestGpsInfo_get_atCgpsinf2:
			if(fsmManager_isStateIn(&gsmModule_requestGpsInfo_state)) {
				fsmManager_stateIn(&gsmModule_requestGpsInfo_state);

				softTimer_start(&timeout, 1000);
			}

			if(string_containsWithinLength(gsmRxDataChunk, (uint8_t *) gsmModule_response_gpsInf2, gsmRxDataChunkLen)) {
				countGpsInfo++;

				if(countGpsInfo >= COUNTGPSINFO_MAX) {
					flags_gsmModule.bits.isGpsFixed = 1;
					flags_gsmModule.bits.requestGpsInfo = 0;

					string_writeStr(gsmInfo, &gsmRxDataChunk[10]);
					if(gsmModuleCallback != NULL) {
						gsmModuleCallback(__gsmModuleEvent_okGpsInfo, (uint8_t *) gsmInfo);
					}

					fsmManager_gotoState(&gsmModule_requestGpsInfo_state, __gsmModule_requestGpsInfo_idle);
				}
				else {
					fsmManager_gotoState(&gsmModule_requestGpsInfo_state, __gsmModule_requestGpsInfo_send_atCgpsinf2);
				}
			}
			else if(softTimer_expired(&timeout)) {
				fsmManager_gotoState(&gsmModule_requestGpsInfo_state, __gsmModule_requestGpsInfo_send_atCgpsinf2);
			}

			if(fsmManager_isStateOut(&gsmModule_requestGpsInfo_state)) {
				fsmManager_stateOut(&gsmModule_requestGpsInfo_state);
			}
			break;



		case __gsmModule_requestGpsInfo_error:
			if(fsmManager_isStateIn(&gsmModule_requestGpsInfo_state)) {
				fsmManager_stateIn(&gsmModule_requestGpsInfo_state);
			}

			flags_gsmModule.bits.requestGpsInfo = 0;
			flags_gsmModuleError.bits.gpsInfo = 1;
			fsmManager_gotoState(&gsmModule_requestGpsInfo_state, __gsmModule_requestGpsInfo_idle);

			if(fsmManager_isStateOut(&gsmModule_requestGpsInfo_state)) {
				fsmManager_stateOut(&gsmModule_requestGpsInfo_state);
			}
			break;



		default:
			if(fsmManager_isStateIn(&gsmModule_requestGpsInfo_state)) {
				fsmManager_stateIn(&gsmModule_requestGpsInfo_state);
			}

			fsmManager_gotoState(&gsmModule_requestGpsInfo_state, __gsmModule_requestGpsInfo_idle);

			if(fsmManager_isStateOut(&gsmModule_requestGpsInfo_state)) {
				fsmManager_stateOut(&gsmModule_requestGpsInfo_state);
			}
			break;
	}
}

static void handle_requestGpsOff(void)
{
	switch(fsmManager_getState(&gsmModule_requestGpsOff_state)) {
		case __gsmModule_requestGpsOff_idle:
			if(fsmManager_isStateIn(&gsmModule_requestGpsOff_state)) {
				fsmManager_stateIn(&gsmModule_requestGpsOff_state);
			}

			fsmManager_gotoState(&gsmModule_requestGpsOff_state, __gsmModule_requestGpsOff_send_atCgpsrst0);

			if(fsmManager_isStateOut(&gsmModule_requestGpsOff_state)) {
				fsmManager_stateOut(&gsmModule_requestGpsOff_state);
			}
			break;



		case __gsmModule_requestGpsOff_send_atCgpsrst0:
			if(fsmManager_isStateIn(&gsmModule_requestGpsOff_state)) {
				fsmManager_stateIn(&gsmModule_requestGpsOff_state);

				softTimer_start(&timeout, 100);
			}

			if(softTimer_expired(&timeout)) {
				pinGsmUartTx_transmit((uint8_t *) gsmModule_command_cgpsrst0);
				fsmManager_gotoState(&gsmModule_requestGpsOff_state, __gsmModule_requestGpsOff_waitOk_atCgpsrst0);
			}

			if(fsmManager_isStateOut(&gsmModule_requestGpsOff_state)) {
				fsmManager_stateOut(&gsmModule_requestGpsOff_state);
			}
			break;



		case __gsmModule_requestGpsOff_waitOk_atCgpsrst0:
			if(fsmManager_isStateIn(&gsmModule_requestGpsOff_state)) {
				fsmManager_stateIn(&gsmModule_requestGpsOff_state);

				softTimer_start(&timeout, 100);
			}

			if(string_containsWithinLength(gsmRxDataChunk, (uint8_t *) gsmModule_response_ok, gsmRxDataChunkLen)) {
				fsmManager_gotoState(&gsmModule_requestGpsOff_state, __gsmModule_requestGpsOff_send_atCgpsrst1);
			}
			else if(softTimer_expired(&timeout)) {
				fsmManager_gotoState(&gsmModule_requestGpsOff_state, __gsmModule_requestGpsOff_send_atCgpsrst0);
			}

			if(fsmManager_isStateOut(&gsmModule_requestGpsOff_state)) {
				fsmManager_stateOut(&gsmModule_requestGpsOff_state);
			}
			break;



		case __gsmModule_requestGpsOff_send_atCgpsrst1:
			if(fsmManager_isStateIn(&gsmModule_requestGpsOff_state)) {
				fsmManager_stateIn(&gsmModule_requestGpsOff_state);

				softTimer_start(&timeout, 100);
			}

			if(softTimer_expired(&timeout)) {
				pinGsmUartTx_transmit((uint8_t *) gsmModule_command_cgpsrst1);
				fsmManager_gotoState(&gsmModule_requestGpsOff_state, __gsmModule_requestGpsOff_waitOk_atCgpsrst1);
			}

			if(fsmManager_isStateOut(&gsmModule_requestGpsOff_state)) {
				fsmManager_stateOut(&gsmModule_requestGpsOff_state);
			}
			break;



		case __gsmModule_requestGpsOff_waitOk_atCgpsrst1:
			if(fsmManager_isStateIn(&gsmModule_requestGpsOff_state)) {
				fsmManager_stateIn(&gsmModule_requestGpsOff_state);

				softTimer_start(&timeout, 100);
			}

			if(string_containsWithinLength(gsmRxDataChunk, (uint8_t *) gsmModule_response_ok, gsmRxDataChunkLen)) {
				fsmManager_gotoState(&gsmModule_requestGpsOff_state, __gsmModule_requestGpsOff_send_atCgpspwr0);
			}
			else if(softTimer_expired(&timeout)) {
				fsmManager_gotoState(&gsmModule_requestGpsOff_state, __gsmModule_requestGpsOff_send_atCgpsrst1);
			}

			if(fsmManager_isStateOut(&gsmModule_requestGpsOff_state)) {
				fsmManager_stateOut(&gsmModule_requestGpsOff_state);
			}
			break;



		case __gsmModule_requestGpsOff_send_atCgpspwr0:
			if(fsmManager_isStateIn(&gsmModule_requestGpsOff_state)) {
				fsmManager_stateIn(&gsmModule_requestGpsOff_state);

				softTimer_start(&timeout, 100);
			}

			if(softTimer_expired(&timeout)) {
				pinGsmUartTx_transmit((uint8_t *) gsmModule_command_cgpspwr0);
				fsmManager_gotoState(&gsmModule_requestGpsOff_state, __gsmModule_requestGpsOff_waitOk_atCgpspwr0);
			}

			if(fsmManager_isStateOut(&gsmModule_requestGpsOff_state)) {
				fsmManager_stateOut(&gsmModule_requestGpsOff_state);
			}
			break;



		case __gsmModule_requestGpsOff_waitOk_atCgpspwr0:
			if(fsmManager_isStateIn(&gsmModule_requestGpsOff_state)) {
				fsmManager_stateIn(&gsmModule_requestGpsOff_state);

				softTimer_start(&timeout, 100);
			}

			if(string_containsWithinLength(gsmRxDataChunk, (uint8_t *) gsmModule_response_ok, gsmRxDataChunkLen)) {
				flags_gsmModule.bits.requestGpsOff = 0;
				flags_gsmModule.bits.isGpsOn = 0;

				fsmManager_gotoState(&gsmModule_requestGpsOff_state, __gsmModule_requestGpsOff_idle);
			}
			else if(softTimer_expired(&timeout)) {
				fsmManager_gotoState(&gsmModule_requestGpsOff_state, __gsmModule_requestGpsOff_send_atCgpspwr0);
			}

			if(fsmManager_isStateOut(&gsmModule_requestGpsOff_state)) {
				fsmManager_stateOut(&gsmModule_requestGpsOff_state);
			}
			break;



		case __gsmModule_requestGpsOff_error:
			if(fsmManager_isStateIn(&gsmModule_requestGpsOff_state)) {
				fsmManager_stateIn(&gsmModule_requestGpsOff_state);
			}

			flags_gsmModuleError.bits.gpsOff = 1;
			flags_gsmModule.bits.requestGpsOff = 0;

			fsmManager_gotoState(&gsmModule_requestGpsOff_state, __gsmModule_requestGpsOn_idle);

			if(fsmManager_isStateOut(&gsmModule_requestGpsOff_state)) {
				fsmManager_stateOut(&gsmModule_requestGpsOff_state);
			}
			break;



		default:
			if(fsmManager_isStateIn(&gsmModule_requestGpsOff_state)) {
				fsmManager_stateIn(&gsmModule_requestGpsOff_state);
			}

			fsmManager_gotoState(&gsmModule_requestGpsOff_state, __gsmModule_requestGpsOn_idle);

			if(fsmManager_isStateOut(&gsmModule_requestGpsOff_state)) {
				fsmManager_stateOut(&gsmModule_requestGpsOff_state);
			}
			break;
	}
}

static void handle_requestServerConnection(void)
{
	switch(fsmManager_getState(&gsmModule_requestServerConnection_state)) {
		case __gsmModule_requestServerConnection_idle:
			if(fsmManager_isStateIn(&gsmModule_requestServerConnection_state)) {
				fsmManager_stateIn(&gsmModule_requestServerConnection_state);
			}

			fsmManager_gotoState(&gsmModule_requestServerConnection_state, __gsmModule_requestServerConnection_send_atCmee1);

			if(fsmManager_isStateOut(&gsmModule_requestServerConnection_state)) {
				fsmManager_stateOut(&gsmModule_requestServerConnection_state);
			}
			break;



		case __gsmModule_requestServerConnection_send_atCmee1:
			if(fsmManager_isStateIn(&gsmModule_requestServerConnection_state)) {
				fsmManager_stateIn(&gsmModule_requestServerConnection_state);
			}

			pinGsmUartTx_transmit((uint8_t *) gsmModule_command_atcmee1);
			fsmManager_gotoState(&gsmModule_requestServerConnection_state, __gsmModule_requestServerConnection_waitOk_atCmee1);

			if(fsmManager_isStateOut(&gsmModule_requestServerConnection_state)) {
				fsmManager_stateOut(&gsmModule_requestServerConnection_state);
			}
			break;



		case __gsmModule_requestServerConnection_waitOk_atCmee1:
			if(fsmManager_isStateIn(&gsmModule_requestServerConnection_state)) {
				fsmManager_stateIn(&gsmModule_requestServerConnection_state);

				softTimer_start(&timeout, 100);
			}

			if(string_containsWithinLength(gsmRxDataChunk, (uint8_t *) gsmModule_response_ok, gsmRxDataChunkLen)) {
				fsmManager_gotoState(&gsmModule_requestServerConnection_state, __gsmModule_requestServerConnection_send_atCfun0);
			}
			else if(softTimer_expired(&timeout)) {
				fsmManager_gotoState(&gsmModule_requestServerConnection_state, __gsmModule_requestServerConnection_send_atCfun0);
			}

			if(fsmManager_isStateOut(&gsmModule_requestServerConnection_state)) {
				fsmManager_stateOut(&gsmModule_requestServerConnection_state);
			}
			break;



		case __gsmModule_requestServerConnection_send_atCfun0:
			if(fsmManager_isStateIn(&gsmModule_requestServerConnection_state)) {
				fsmManager_stateIn(&gsmModule_requestServerConnection_state);

				softTimer_start(&timeout, 100);
			}

			if(softTimer_expired(&timeout)) {
				pinGsmUartTx_transmit((uint8_t *) gsmModule_command_cfun0);
				fsmManager_gotoState(&gsmModule_requestServerConnection_state, __gsmModule_requestServerConnection_waitOk_atCfun0);
			}

			if(fsmManager_isStateOut(&gsmModule_requestServerConnection_state)) {
				fsmManager_stateOut(&gsmModule_requestServerConnection_state);
			}
			break;



		case __gsmModule_requestServerConnection_waitOk_atCfun0:
			if(fsmManager_isStateIn(&gsmModule_requestServerConnection_state)) {
				fsmManager_stateIn(&gsmModule_requestServerConnection_state);

				softTimer_start(&timeout, 10*1000);
			}

			if(string_containsWithinLength(gsmRxDataChunk, (uint8_t *) gsmModule_response_ok, gsmRxDataChunkLen)) {
				fsmManager_gotoState(&gsmModule_requestServerConnection_state, __gsmModule_requestServerConnection_send_atCfun1);
			}
			else if(softTimer_expired(&timeout)) {
				fsmManager_gotoState(&gsmModule_requestServerConnection_state, __gsmModule_requestServerConnection_send_atCfun0);
			}

			if(fsmManager_isStateOut(&gsmModule_requestServerConnection_state)) {
				fsmManager_stateOut(&gsmModule_requestServerConnection_state);
			}
			break;



		case __gsmModule_requestServerConnection_send_atCfun1:
			if(fsmManager_isStateIn(&gsmModule_requestServerConnection_state)) {
				fsmManager_stateIn(&gsmModule_requestServerConnection_state);

				softTimer_start(&timeout, 100);
			}

			if(softTimer_expired(&timeout)) {
				pinGsmUartTx_transmit((uint8_t *) gsmModule_command_cfun1);
				fsmManager_gotoState(&gsmModule_requestServerConnection_state, __gsmModule_requestServerConnection_waitOk_atCfun1);
			}

			if(fsmManager_isStateOut(&gsmModule_requestServerConnection_state)) {
				fsmManager_stateOut(&gsmModule_requestServerConnection_state);
			}
			break;



		case __gsmModule_requestServerConnection_waitOk_atCfun1:
			if(fsmManager_isStateIn(&gsmModule_requestServerConnection_state)) {
				fsmManager_stateIn(&gsmModule_requestServerConnection_state);

				softTimer_start(&timeout, 10*1000);
			}

			if(string_containsWithinLength(gsmRxDataChunk, (uint8_t *) gsmModule_response_ok, gsmRxDataChunkLen)) {
				fsmManager_gotoState(&gsmModule_requestServerConnection_state, __gsmModule_requestServerConnection_send_atCpin);
			}
			else if(softTimer_expired(&timeout)) {
				fsmManager_gotoState(&gsmModule_requestServerConnection_state, __gsmModule_requestServerConnection_send_atCfun1);
			}

			if(fsmManager_isStateOut(&gsmModule_requestServerConnection_state)) {
				fsmManager_stateOut(&gsmModule_requestServerConnection_state);
			}
			break;



		case __gsmModule_requestServerConnection_send_atCpin:
			if(fsmManager_isStateIn(&gsmModule_requestServerConnection_state)) {
				fsmManager_stateIn(&gsmModule_requestServerConnection_state);

				softTimer_start(&timeout, 100);
			}

			if(softTimer_expired(&timeout)) {
				pinGsmUartTx_transmit((uint8_t *) gsmModule_command_cpin);
				fsmManager_gotoState(&gsmModule_requestServerConnection_state, __gsmModule_requestServerConnection_check_atCpin);
			}

			if(fsmManager_isStateOut(&gsmModule_requestServerConnection_state)) {
				fsmManager_stateOut(&gsmModule_requestServerConnection_state);
			}
			break;



		case __gsmModule_requestServerConnection_check_atCpin:
			if(fsmManager_isStateIn(&gsmModule_requestServerConnection_state)) {
				fsmManager_stateIn(&gsmModule_requestServerConnection_state);

				softTimer_start(&timeout, 5*1000);
			}

			if(string_containsWithinLength(gsmRxDataChunk, (uint8_t *) gsmModule_response_cpinReady, gsmRxDataChunkLen)) {
				fsmManager_gotoState(&gsmModule_requestServerConnection_state, __gsmModule_requestServerConnection_send_atCband);
			}
			else if(string_containsWithinLength(gsmRxDataChunk, (uint8_t *) gsmModule_response_cme_error, gsmRxDataChunkLen)) {
				fsmManager_gotoState(&gsmModule_requestServerConnection_state, __gsmModule_requestServerConnection_send_atCfun1);
			}
			else if(softTimer_expired(&timeout)) {
				fsmManager_gotoState(&gsmModule_requestServerConnection_state, __gsmModule_requestServerConnection_send_atCpin);
			}


			if(fsmManager_isStateOut(&gsmModule_requestServerConnection_state)) {
				fsmManager_stateOut(&gsmModule_requestServerConnection_state);
			}
			break;



		case __gsmModule_requestServerConnection_send_atCband:
			if(fsmManager_isStateIn(&gsmModule_requestServerConnection_state)) {
				fsmManager_stateIn(&gsmModule_requestServerConnection_state);

				softTimer_start(&timeout, 1000);
			}

			if(softTimer_expired(&timeout)) {
				pinGsmUartTx_transmit((uint8_t *) gsmModule_command_cband_allBand);
				fsmManager_gotoState(&gsmModule_requestServerConnection_state, __gsmModule_requestServerConnection_waitOk_atCband);
			}

			if(fsmManager_isStateOut(&gsmModule_requestServerConnection_state)) {
				fsmManager_stateOut(&gsmModule_requestServerConnection_state);
			}
			break;



		case __gsmModule_requestServerConnection_waitOk_atCband:
			if(fsmManager_isStateIn(&gsmModule_requestServerConnection_state)) {
				fsmManager_stateIn(&gsmModule_requestServerConnection_state);

				softTimer_start(&timeout, 5000);
			}

			if(string_containsWithinLength(gsmRxDataChunk, (uint8_t *) gsmModule_response_ok, gsmRxDataChunkLen)) {
				fsmManager_gotoState(&gsmModule_requestServerConnection_state, __gsmModule_requestServerConnection_send_atCreg1);
			}
			else if(softTimer_expired(&timeout)) {
				fsmManager_gotoState(&gsmModule_requestServerConnection_state, __gsmModule_requestServerConnection_send_atCband);
			}

			if(fsmManager_isStateOut(&gsmModule_requestServerConnection_state)) {
				fsmManager_stateOut(&gsmModule_requestServerConnection_state);
			}
			break;



		case __gsmModule_requestServerConnection_send_atCreg1:
			if(fsmManager_isStateIn(&gsmModule_requestServerConnection_state)) {
				fsmManager_stateIn(&gsmModule_requestServerConnection_state);

				softTimer_start(&timeout, 100);
			}

			if(softTimer_expired(&timeout)) {
				pinGsmUartTx_transmit((uint8_t *) gsmModule_command_creg1);
				fsmManager_gotoState(&gsmModule_requestServerConnection_state, __gsmModule_requestServerConnection_check_atCreg1);
				//fsmManager_gotoState(&gsmModule_requestServerConnection_state, __gsmModule_requestServerConnection_waitOk_atCreg1);
			}

			if(fsmManager_isStateOut(&gsmModule_requestServerConnection_state)) {
				fsmManager_stateOut(&gsmModule_requestServerConnection_state);
			}
			break;



		case __gsmModule_requestServerConnection_check_atCreg1:
			if(fsmManager_isStateIn(&gsmModule_requestServerConnection_state)) {
				fsmManager_stateIn(&gsmModule_requestServerConnection_state);

				softTimer_start(&timeout, 20*1000);
			}

			if(flags_gsmModuleUnsolicited.bits.creg1) {
				flags_gsmModuleUnsolicited.bits.creg1 = 0;

				fsmManager_gotoState(&gsmModule_requestServerConnection_state, __gsmModule_requestServerConnection_send_atCops);
			}
			else if(flags_gsmModuleUnsolicited.bits.creg2) {
				flags_gsmModuleUnsolicited.bits.creg2 = 0;

				fsmManager_gotoState(&gsmModule_requestServerConnection_state, __gsmModule_requestServerConnection_send_atCreg1);
			}
			else if(softTimer_expired(&timeout) || flags_gsmModuleUnsolicited.bits.creg0) {
				flags_gsmModuleUnsolicited.bits.creg0 = 0;

				fsmManager_gotoState(&gsmModule_requestServerConnection_state, __gsmModule_requestServerConnection_send_atCreg1);
			}


			if(fsmManager_isStateOut(&gsmModule_requestServerConnection_state)) {
				fsmManager_stateOut(&gsmModule_requestServerConnection_state);
			}
			break;



		case __gsmModule_requestServerConnection_waitOk_atCreg1:
			if(fsmManager_isStateIn(&gsmModule_requestServerConnection_state)) {
				fsmManager_stateIn(&gsmModule_requestServerConnection_state);

				softTimer_start(&timeout, 2*1000);
			}

			if(string_containsWithinLength(gsmRxDataChunk, (uint8_t *) gsmModule_response_ok, gsmRxDataChunkLen)) {
				fsmManager_gotoState(&gsmModule_requestServerConnection_state, __gsmModule_requestServerConnection_send_atCreg);
			}
			else if(softTimer_expired(&timeout)) {
				fsmManager_gotoState(&gsmModule_requestServerConnection_state, __gsmModule_requestServerConnection_send_atCreg1);
			}

			if(fsmManager_isStateOut(&gsmModule_requestServerConnection_state)) {
				fsmManager_stateOut(&gsmModule_requestServerConnection_state);
			}
			break;



		case __gsmModule_requestServerConnection_send_atCreg:
			if(fsmManager_isStateIn(&gsmModule_requestServerConnection_state)) {
				fsmManager_stateIn(&gsmModule_requestServerConnection_state);

				softTimer_start(&timeout, 1000);
			}


			if(softTimer_expired(&timeout)) {
				pinGsmUartTx_transmit((uint8_t *) gsmModule_command_creg);
				fsmManager_gotoState(&gsmModule_requestServerConnection_state, __gsmModule_requestServerConnection_check_atCreg);
			}

			if(fsmManager_isStateOut(&gsmModule_requestServerConnection_state)) {
				fsmManager_stateOut(&gsmModule_requestServerConnection_state);
			}
			break;



		case __gsmModule_requestServerConnection_check_atCreg:
			if(fsmManager_isStateIn(&gsmModule_requestServerConnection_state)) {
				fsmManager_stateIn(&gsmModule_requestServerConnection_state);

				softTimer_start(&timeout, 5*1000);
			}

			if(string_containsWithinLength(gsmRxDataChunk, (uint8_t *) gsmModule_response_creg1_1, gsmRxDataChunkLen)) {
				fsmManager_gotoState(&gsmModule_requestServerConnection_state, __gsmModule_requestServerConnection_send_atCops);
			}
			else if(string_containsWithinLength(gsmRxDataChunk, (uint8_t *) gsmModule_response_creg1_0, gsmRxDataChunkLen)) {
				fsmManager_gotoState(&gsmModule_requestServerConnection_state, __gsmModule_requestServerConnection_send_atCreg);
			}
			else if(softTimer_expired(&timeout)) {
				fsmManager_gotoState(&gsmModule_requestServerConnection_state, __gsmModule_requestServerConnection_send_atCreg);
			}

			if(fsmManager_isStateOut(&gsmModule_requestServerConnection_state)) {
				fsmManager_stateOut(&gsmModule_requestServerConnection_state);
			}
			break;



		case __gsmModule_requestServerConnection_send_atCops:
			if(fsmManager_isStateIn(&gsmModule_requestServerConnection_state)) {
				fsmManager_stateIn(&gsmModule_requestServerConnection_state);

				softTimer_start(&timeout, 100);
			}

			if(softTimer_expired(&timeout)) {
				pinGsmUartTx_transmit((uint8_t *) gsmModule_command_cops);
				fsmManager_gotoState(&gsmModule_requestServerConnection_state, __gsmModule_requestServerConnection_get_atCops);
			}

			if(fsmManager_isStateOut(&gsmModule_requestServerConnection_state)) {
				fsmManager_stateOut(&gsmModule_requestServerConnection_state);
			}
			break;



		case __gsmModule_requestServerConnection_get_atCops:
			if(fsmManager_isStateIn(&gsmModule_requestServerConnection_state)) {
				fsmManager_stateIn(&gsmModule_requestServerConnection_state);

				softTimer_start(&timeout, 45*1000);
			}

			if(string_containsWithinLength(gsmRxDataChunk, (uint8_t *) "PERSONAL", gsmRxDataChunkLen)) {
				gsmModule_operator = __gsmModule_operator_personal;

				fsmManager_gotoState(&gsmModule_requestServerConnection_state, __gsmModule_requestServerConnection_send_atSapbr3_contype);
			}
			else if(string_containsWithinLength(gsmRxDataChunk, (uint8_t *) "CLARO", gsmRxDataChunkLen)) {
				gsmModule_operator = __gsmModule_operator_claro;

				fsmManager_gotoState(&gsmModule_requestServerConnection_state, __gsmModule_requestServerConnection_send_atSapbr3_contype);
			}
			else if(string_containsWithinLength(gsmRxDataChunk, (uint8_t *) "MOVISTAR", gsmRxDataChunkLen)) {
				gsmModule_operator = __gsmModule_operator_movistar;

				fsmManager_gotoState(&gsmModule_requestServerConnection_state, __gsmModule_requestServerConnection_send_atSapbr3_contype);
			}
			else if(softTimer_expired(&timeout)) {
				gsmModule_operator = __gsmModule_operator_unknown;

				fsmManager_gotoState(&gsmModule_requestServerConnection_state, __gsmModule_requestServerConnection_send_atCops);
			}

			if(fsmManager_isStateOut(&gsmModule_requestServerConnection_state)) {
				fsmManager_stateOut(&gsmModule_requestServerConnection_state);
			}
			break;



		case __gsmModule_requestServerConnection_send_atSapbr3_contype:
			if(fsmManager_isStateIn(&gsmModule_requestServerConnection_state)) {
				fsmManager_stateIn(&gsmModule_requestServerConnection_state);

				softTimer_start(&timeout, 100);
			}

			if(softTimer_expired(&timeout)) {
				pinGsmUartTx_transmit((uint8_t *) gsmModule_command_sapbr3contype);
				fsmManager_gotoState(&gsmModule_requestServerConnection_state, __gsmModule_requestServerConnection_waitOk_atSapbr3_contype);
			}

			if(fsmManager_isStateOut(&gsmModule_requestServerConnection_state)) {
				fsmManager_stateOut(&gsmModule_requestServerConnection_state);
			}
			break;



		case __gsmModule_requestServerConnection_waitOk_atSapbr3_contype:
			if(fsmManager_isStateIn(&gsmModule_requestServerConnection_state)) {
				fsmManager_stateIn(&gsmModule_requestServerConnection_state);

				softTimer_start(&timeout, 1*1000);
			}

			if(string_containsWithinLength(gsmRxDataChunk, (uint8_t *) gsmModule_response_ok, gsmRxDataChunkLen)) {
				fsmManager_gotoState(&gsmModule_requestServerConnection_state, __gsmModule_requestServerConnection_send_atSapbr3_apn);
			}
			else if(softTimer_expired(&timeout)) {
				fsmManager_gotoState(&gsmModule_requestServerConnection_state, __gsmModule_requestServerConnection_send_atSapbr3_contype);
			}


			if(fsmManager_isStateOut(&gsmModule_requestServerConnection_state)) {
				fsmManager_stateOut(&gsmModule_requestServerConnection_state);
			}
			break;



		case __gsmModule_requestServerConnection_send_atSapbr3_apn:
			if(fsmManager_isStateIn(&gsmModule_requestServerConnection_state)) {
				fsmManager_stateIn(&gsmModule_requestServerConnection_state);

				softTimer_start(&timeout, 100);
			}

			if(softTimer_expired(&timeout)) {
				if(gsmModule_operator == __gsmModule_operator_personal) {
					pinGsmUartTx_transmit((uint8_t *) gsmModule_command_sapbr3apnPersonal);
					fsmManager_gotoState(&gsmModule_requestServerConnection_state, __gsmModule_requestServerConnection_waitOk_atSapbr3_apn);
				}
			}

			if(fsmManager_isStateOut(&gsmModule_requestServerConnection_state)) {
				fsmManager_stateOut(&gsmModule_requestServerConnection_state);
			}
			break;



		case __gsmModule_requestServerConnection_waitOk_atSapbr3_apn:
			if(fsmManager_isStateIn(&gsmModule_requestServerConnection_state)) {
				fsmManager_stateIn(&gsmModule_requestServerConnection_state);

				softTimer_start(&timeout, 1000);
			}

			if(string_containsWithinLength(gsmRxDataChunk, (uint8_t *) gsmModule_response_ok, gsmRxDataChunkLen)) {
				fsmManager_gotoState(&gsmModule_requestServerConnection_state, __gsmModule_requestServerConnection_send_atSapbr3_user);
			}
			else if(softTimer_expired(&timeout)) {
				fsmManager_gotoState(&gsmModule_requestServerConnection_state, __gsmModule_requestServerConnection_send_atSapbr3_apn);
			}


			if(fsmManager_isStateOut(&gsmModule_requestServerConnection_state)) {
				fsmManager_stateOut(&gsmModule_requestServerConnection_state);
			}
			break;



		case __gsmModule_requestServerConnection_send_atSapbr3_user:
			if(fsmManager_isStateIn(&gsmModule_requestServerConnection_state)) {
				fsmManager_stateIn(&gsmModule_requestServerConnection_state);

				softTimer_start(&timeout, 100);
			}

			if(softTimer_expired(&timeout)) {
				if(gsmModule_operator == __gsmModule_operator_personal) {
					pinGsmUartTx_transmit((uint8_t *) gsmModule_command_sapbr3userPersonal);
					fsmManager_gotoState(&gsmModule_requestServerConnection_state, __gsmModule_requestServerConnection_waitOk_atSapbr3_user);
				}
			}


			if(fsmManager_isStateOut(&gsmModule_requestServerConnection_state)) {
				fsmManager_stateOut(&gsmModule_requestServerConnection_state);
			}
			break;



		case __gsmModule_requestServerConnection_waitOk_atSapbr3_user:
			if(fsmManager_isStateIn(&gsmModule_requestServerConnection_state)) {
				fsmManager_stateIn(&gsmModule_requestServerConnection_state);

				softTimer_start(&timeout, 1000);
			}

			if(string_containsWithinLength(gsmRxDataChunk, (uint8_t *) gsmModule_response_ok, gsmRxDataChunkLen)) {
				fsmManager_gotoState(&gsmModule_requestServerConnection_state, __gsmModule_requestServerConnection_send_atSapbr3_pwd);
			}
			else if(softTimer_expired(&timeout)) {
				fsmManager_gotoState(&gsmModule_requestServerConnection_state, __gsmModule_requestServerConnection_send_atSapbr3_user);
			}


			if(fsmManager_isStateOut(&gsmModule_requestServerConnection_state)) {
				fsmManager_stateOut(&gsmModule_requestServerConnection_state);
			}
			break;



		case __gsmModule_requestServerConnection_send_atSapbr3_pwd:
			if(fsmManager_isStateIn(&gsmModule_requestServerConnection_state)) {
				fsmManager_stateIn(&gsmModule_requestServerConnection_state);

				softTimer_start(&timeout, 100);
			}

			if(softTimer_expired(&timeout)) {
				if(gsmModule_operator == __gsmModule_operator_personal) {
					pinGsmUartTx_transmit((uint8_t *) gsmModule_command_sapbr3pwdPersonal);
					fsmManager_gotoState(&gsmModule_requestServerConnection_state, __gsmModule_requestServerConnection_waitOk_atSapbr3_pwd);
				}
			}

			if(fsmManager_isStateOut(&gsmModule_requestServerConnection_state)) {
				fsmManager_stateOut(&gsmModule_requestServerConnection_state);
			}
			break;



		case __gsmModule_requestServerConnection_waitOk_atSapbr3_pwd:
			if(fsmManager_isStateIn(&gsmModule_requestServerConnection_state)) {
				fsmManager_stateIn(&gsmModule_requestServerConnection_state);

				softTimer_start(&timeout, 3*1000);
			}

			if(string_containsWithinLength(gsmRxDataChunk, (uint8_t *) gsmModule_response_ok, gsmRxDataChunkLen)) {
				fsmManager_gotoState(&gsmModule_requestServerConnection_state, __gsmModule_requestServerConnection_send_atSapbr1);
			}
			else if(softTimer_expired(&timeout)) {
				fsmManager_gotoState(&gsmModule_requestServerConnection_state, __gsmModule_requestServerConnection_send_atSapbr3_pwd);
			}


			if(fsmManager_isStateOut(&gsmModule_requestServerConnection_state)) {
				fsmManager_stateOut(&gsmModule_requestServerConnection_state);
			}
			break;



		case __gsmModule_requestServerConnection_send_atSapbr1:
			if(fsmManager_isStateIn(&gsmModule_requestServerConnection_state)) {
				fsmManager_stateIn(&gsmModule_requestServerConnection_state);

				softTimer_start(&timeout, 100);
			}

			if(softTimer_expired(&timeout)) {
				pinGsmUartTx_transmit((uint8_t *) gsmModule_command_sapbr1);
				fsmManager_gotoState(&gsmModule_requestServerConnection_state, __gsmModule_requestServerConnection_waitOk_atSapbr1);
			}

			if(fsmManager_isStateOut(&gsmModule_requestServerConnection_state)) {
				fsmManager_stateOut(&gsmModule_requestServerConnection_state);
			}
			break;



		case __gsmModule_requestServerConnection_waitOk_atSapbr1:
			if(fsmManager_isStateIn(&gsmModule_requestServerConnection_state)) {
				fsmManager_stateIn(&gsmModule_requestServerConnection_state);

				softTimer_start(&timeout, 20*1000);
			}

			if(string_containsWithinLength(gsmRxDataChunk, (uint8_t *) gsmModule_response_ok, gsmRxDataChunkLen)) {
				fsmManager_gotoState(&gsmModule_requestServerConnection_state, __gsmModule_requestServerConnection_send_atSapbr2);
			}
			else if(softTimer_expired(&timeout)) {
				fsmManager_gotoState(&gsmModule_requestServerConnection_state, __gsmModule_requestServerConnection_send_atSapbr1);
			}

			if(fsmManager_isStateOut(&gsmModule_requestServerConnection_state)) {
				fsmManager_stateOut(&gsmModule_requestServerConnection_state);
			}
			break;



		case __gsmModule_requestServerConnection_send_atSapbr2:
			if(fsmManager_isStateIn(&gsmModule_requestServerConnection_state)) {
				fsmManager_stateIn(&gsmModule_requestServerConnection_state);

				softTimer_start(&timeout, 100);
			}

			if(softTimer_expired(&timeout)) {
				pinGsmUartTx_transmit((uint8_t *) gsmModule_command_sapbr2);
				fsmManager_gotoState(&gsmModule_requestServerConnection_state, __gsmModule_requestServerConnection_check_atSapbr2);
			}

			if(fsmManager_isStateOut(&gsmModule_requestServerConnection_state)) {
				fsmManager_stateOut(&gsmModule_requestServerConnection_state);
			}
			break;



		case __gsmModule_requestServerConnection_check_atSapbr2:
			if(fsmManager_isStateIn(&gsmModule_requestServerConnection_state)) {
				fsmManager_stateIn(&gsmModule_requestServerConnection_state);

				softTimer_start(&timeout, 1000);
			}

			if(!string_containsWithinLength(gsmRxDataChunk, (uint8_t *) "0.0.0.0", gsmRxDataChunkLen)) {
				fsmManager_gotoState(&gsmModule_requestServerConnection_state, __gsmModule_requestServerConnection_send_atHttpinit);
			}
			else if(softTimer_expired(&timeout)) {
				fsmManager_gotoState(&gsmModule_requestServerConnection_state, __gsmModule_requestServerConnection_send_atSapbr2);
			}

			if(fsmManager_isStateOut(&gsmModule_requestServerConnection_state)) {
				fsmManager_stateOut(&gsmModule_requestServerConnection_state);
			}
			break;



		case __gsmModule_requestServerConnection_send_atHttpinit:
			if(fsmManager_isStateIn(&gsmModule_requestServerConnection_state)) {
				fsmManager_stateIn(&gsmModule_requestServerConnection_state);

				softTimer_start(&timeout, 100);
			}

			if(softTimer_expired(&timeout)) {
				pinGsmUartTx_transmit((uint8_t *) gsmModule_command_httpinit);
				fsmManager_gotoState(&gsmModule_requestServerConnection_state, __gsmModule_requestServerConnection_waitOk_atHttpinit);
			}

			if(fsmManager_isStateOut(&gsmModule_requestServerConnection_state)) {
				fsmManager_stateOut(&gsmModule_requestServerConnection_state);
			}
			break;



		case __gsmModule_requestServerConnection_waitOk_atHttpinit:
			if(fsmManager_isStateIn(&gsmModule_requestServerConnection_state)) {
				fsmManager_stateIn(&gsmModule_requestServerConnection_state);

				softTimer_start(&timeout, 100);
			}

			if(string_containsWithinLength(gsmRxDataChunk, (uint8_t *) gsmModule_response_ok, gsmRxDataChunkLen)) {
				fsmManager_gotoState(&gsmModule_requestServerConnection_state, __gsmModule_requestServerConnection_send_atHttppara_cid);
			}
			else if(softTimer_expired(&timeout)) {
				fsmManager_gotoState(&gsmModule_requestServerConnection_state, __gsmModule_requestServerConnection_send_atHttpinit);
			}

			if(fsmManager_isStateOut(&gsmModule_requestServerConnection_state)) {
				fsmManager_stateOut(&gsmModule_requestServerConnection_state);
			}
			break;



		case __gsmModule_requestServerConnection_send_atHttppara_cid:
			if(fsmManager_isStateIn(&gsmModule_requestServerConnection_state)) {
				fsmManager_stateIn(&gsmModule_requestServerConnection_state);

				softTimer_start(&timeout, 100);
			}

			if(softTimer_expired(&timeout)) {
				pinGsmUartTx_transmit((uint8_t *) gsmModule_command_httppara_cid);
				fsmManager_gotoState(&gsmModule_requestServerConnection_state, __gsmModule_requestServerConnection_waitOk_atHttppara_cid);
			}

			if(fsmManager_isStateOut(&gsmModule_requestServerConnection_state)) {
				fsmManager_stateOut(&gsmModule_requestServerConnection_state);
			}
			break;



		case __gsmModule_requestServerConnection_waitOk_atHttppara_cid:
			if(fsmManager_isStateIn(&gsmModule_requestServerConnection_state)) {
				fsmManager_stateIn(&gsmModule_requestServerConnection_state);

				softTimer_start(&timeout, 2*1000);
			}

			if(string_containsWithinLength(gsmRxDataChunk, (uint8_t *) gsmModule_response_ok, gsmRxDataChunkLen)) {
				fsmManager_gotoState(&gsmModule_requestServerConnection_state, __gsmModule_requestServerConnection_idle);

				flags_gsmModule.bits.isServerConnected = 1;
				flags_gsmModule.bits.requestServerConnection = 0;
			}
			else if(softTimer_expired(&timeout)) {
				fsmManager_gotoState(&gsmModule_requestServerConnection_state, __gsmModule_requestServerConnection_send_atHttppara_cid);
			}

			if(fsmManager_isStateOut(&gsmModule_requestServerConnection_state)) {
				fsmManager_stateOut(&gsmModule_requestServerConnection_state);
			}
			break;



		case __gsmModule_requestServerConnection_error:
			if(fsmManager_isStateIn(&gsmModule_requestServerConnection_state)) {
				fsmManager_stateIn(&gsmModule_requestServerConnection_state);
			}

			flags_gsmModuleError.bits.serverConnection = 1;
			flags_gsmModule.bits.requestServerConnection = 0;

			fsmManager_gotoState(&gsmModule_requestServerConnection_state, __gsmModule_requestServerConnection_idle);

			if(fsmManager_isStateOut(&gsmModule_requestServerConnection_state)) {
				fsmManager_stateOut(&gsmModule_requestServerConnection_state);
			}
			break;



		default:
			if(fsmManager_isStateIn(&gsmModule_requestServerConnection_state)) {
				fsmManager_stateIn(&gsmModule_requestServerConnection_state);
			}

			flags_gsmModule.bits.requestServerConnection = 0;
			fsmManager_gotoState(&gsmModule_requestServerConnection_state, __gsmModule_requestServerConnection_idle);

			if(fsmManager_isStateOut(&gsmModule_requestServerConnection_state)) {
				fsmManager_stateOut(&gsmModule_requestServerConnection_state);
			}
			break;
	}
}

static void handle_requestServerDataSend(void)
{
	switch(fsmManager_getState(&gsmModule_requestServerDataSend_state)) {
		case __gsmModule_requestServerDataSend_idle:
			if(fsmManager_isStateIn(&gsmModule_requestServerDataSend_state)) {
				fsmManager_stateIn(&gsmModule_requestServerDataSend_state);
			}

			fsmManager_gotoState(&gsmModule_requestServerDataSend_state, __gsmModule_requestServerDataSend_send_atHttppara_url);

			if(fsmManager_isStateOut(&gsmModule_requestServerDataSend_state)) {
				fsmManager_stateOut(&gsmModule_requestServerDataSend_state);
			}
			break;

		case __gsmModule_requestServerDataSend_send_atHttppara_url:
			if(fsmManager_isStateIn(&gsmModule_requestServerDataSend_state)) {
				fsmManager_stateIn(&gsmModule_requestServerDataSend_state);

				softTimer_start(&timeout, 100);
			}

			if(softTimer_expired(&timeout)) {
				pinGsmUartTx_transmit((uint8_t *) dataToSend);
				fsmManager_gotoState(&gsmModule_requestServerDataSend_state, __gsmModule_requestServerDataSend_waitOk_atHttppara_url);
			}

			if(fsmManager_isStateOut(&gsmModule_requestServerDataSend_state)) {
				fsmManager_stateOut(&gsmModule_requestServerDataSend_state);
			}
			break;

		case __gsmModule_requestServerDataSend_waitOk_atHttppara_url:
			if(fsmManager_isStateIn(&gsmModule_requestServerDataSend_state)) {
				fsmManager_stateIn(&gsmModule_requestServerDataSend_state);

				softTimer_start(&timeout, 5*1000);
			}

			if(string_containsWithinLength(gsmRxDataChunk, (uint8_t *) gsmModule_response_ok, gsmRxDataChunkLen)) {
				fsmManager_gotoState(&gsmModule_requestServerDataSend_state, __gsmModule_requestServerDataSend_send_atHttpaction1);
			}
			else if(softTimer_expired(&timeout)) {
				fsmManager_gotoState(&gsmModule_requestServerDataSend_state, __gsmModule_requestServerDataSend_send_atHttppara_url);
			}

			if(fsmManager_isStateOut(&gsmModule_requestServerDataSend_state)) {
				fsmManager_stateOut(&gsmModule_requestServerDataSend_state);
			}
			break;

		case __gsmModule_requestServerDataSend_send_atHttpaction1:
			if(fsmManager_isStateIn(&gsmModule_requestServerDataSend_state)) {
				fsmManager_stateIn(&gsmModule_requestServerDataSend_state);

				softTimer_start(&timeout, 100);
			}

			if(softTimer_expired(&timeout)) {
				pinGsmUartTx_transmit((uint8_t *) gsmModule_command_httpaction1);
				fsmManager_gotoState(&gsmModule_requestServerDataSend_state, __gsmModule_requestServerDataSend_waitOk_atHttpaction1);
			}

			if(fsmManager_isStateOut(&gsmModule_requestServerDataSend_state)) {
				fsmManager_stateOut(&gsmModule_requestServerDataSend_state);
			}
			break;

		case __gsmModule_requestServerDataSend_waitOk_atHttpaction1:
			if(fsmManager_isStateIn(&gsmModule_requestServerDataSend_state)) {
				fsmManager_stateIn(&gsmModule_requestServerDataSend_state);

				softTimer_start(&timeout, 120*1000);
			}

			if(flags_gsmModuleUnsolicited.bits.httpActionOk == 1) {
				flags_gsmModule.bits.isServerDataSent = 1;
				flags_gsmModule.bits.requestServerDataSend = 0;

				fsmManager_gotoState(&gsmModule_requestServerDataSend_state, __gsmModule_requestServerDataSend_idle);
			}
			else if(flags_gsmModuleUnsolicited.bits.httpActionError == 1) {
				fsmManager_gotoState(&gsmModule_requestServerDataSend_state, __gsmModule_requestServerDataSend_error);
			}
			else if(softTimer_expired(&timeout)) {
				fsmManager_gotoState(&gsmModule_requestServerDataSend_state, __gsmModule_requestServerDataSend_waitOk_atHttpaction1);
			}

			if(fsmManager_isStateOut(&gsmModule_requestServerDataSend_state)) {
				fsmManager_stateOut(&gsmModule_requestServerDataSend_state);
			}
			break;

		case __gsmModule_requestServerDataSend_error:
			if(fsmManager_isStateIn(&gsmModule_requestServerDataSend_state)) {
				fsmManager_stateIn(&gsmModule_requestServerDataSend_state);
			}

			flags_gsmModuleError.bits.serverSendData = 1;
			flags_gsmModule.bits.requestServerDataSend = 0;

			fsmManager_gotoState(&gsmModule_requestServerDataSend_state, __gsmModule_requestServerDataSend_idle);

			if(fsmManager_isStateOut(&gsmModule_requestServerDataSend_state)) {
				fsmManager_stateOut(&gsmModule_requestServerDataSend_state);
			}
			break;

		default:
			if(fsmManager_isStateIn(&gsmModule_requestServerDataSend_state)) {
				fsmManager_stateIn(&gsmModule_requestServerDataSend_state);
			}

			fsmManager_gotoState(&gsmModule_requestServerDataSend_state, __gsmModule_requestServerDataSend_idle);

			if(fsmManager_isStateOut(&gsmModule_requestServerDataSend_state)) {
				fsmManager_stateOut(&gsmModule_requestServerDataSend_state);
			}
			break;
	}
}

static void handle_requestServerDisconnection(void)
{
	switch(fsmManager_getState(&gsmModule_requestServerDisconnection_state)) {
		case __gsmModule_requestServerDisconnection_idle:
			if(fsmManager_isStateIn(&gsmModule_requestServerDisconnection_state)) {
				fsmManager_stateIn(&gsmModule_requestServerDisconnection_state);
			}

			fsmManager_gotoState(&gsmModule_requestServerDisconnection_state, __gsmModule_requestServerDisconnection_send_atHttpterm);

			if(fsmManager_isStateOut(&gsmModule_requestServerDisconnection_state)) {
				fsmManager_stateOut(&gsmModule_requestServerDisconnection_state);
			}
			break;



		case __gsmModule_requestServerDisconnection_send_atHttpterm:
			if(fsmManager_isStateIn(&gsmModule_requestServerDisconnection_state)) {
				fsmManager_stateIn(&gsmModule_requestServerDisconnection_state);

				softTimer_start(&timeout, 100);
			}

			if(softTimer_expired(&timeout)) {
				pinGsmUartTx_transmit((uint8_t *) gsmModule_command_httpterm);
				fsmManager_gotoState(&gsmModule_requestServerDisconnection_state, __gsmModule_requestServerDisconnection_waitOk_atHttpterm);
			}

			if(fsmManager_isStateOut(&gsmModule_requestServerDisconnection_state)) {
				fsmManager_stateOut(&gsmModule_requestServerDisconnection_state);
			}
			break;



		case __gsmModule_requestServerDisconnection_waitOk_atHttpterm:
			if(fsmManager_isStateIn(&gsmModule_requestServerDisconnection_state)) {
				fsmManager_stateIn(&gsmModule_requestServerDisconnection_state);

				softTimer_start(&timeout, 100);
			}

			if(string_containsWithinLength(gsmRxDataChunk, (uint8_t *) gsmModule_response_ok, gsmRxDataChunkLen)) {
				fsmManager_gotoState(&gsmModule_requestServerDisconnection_state, __gsmModule_requestServerDisconnection_send_atSapbr0);
			}
			else if(softTimer_expired(&timeout)) {
				fsmManager_gotoState(&gsmModule_requestServerDisconnection_state, __gsmModule_requestServerDisconnection_send_atHttpterm);
			}

			if(fsmManager_isStateOut(&gsmModule_requestServerDisconnection_state)) {
				fsmManager_stateOut(&gsmModule_requestServerDisconnection_state);
			}
			break;



		case __gsmModule_requestServerDisconnection_send_atSapbr0:
			if(fsmManager_isStateIn(&gsmModule_requestServerDisconnection_state)) {
				fsmManager_stateIn(&gsmModule_requestServerDisconnection_state);

				softTimer_start(&timeout, 100);
			}

			if(softTimer_expired(&timeout)) {
				pinGsmUartTx_transmit((uint8_t *) gsmModule_command_sapbr0);
				fsmManager_gotoState(&gsmModule_requestServerDisconnection_state, __gsmModule_requestServerDisconnection_waitOk_atSapbr0);
			}

			if(fsmManager_isStateOut(&gsmModule_requestServerDisconnection_state)) {
				fsmManager_stateOut(&gsmModule_requestServerDisconnection_state);
			}
			break;



		case __gsmModule_requestServerDisconnection_waitOk_atSapbr0:
			if(fsmManager_isStateIn(&gsmModule_requestServerDisconnection_state)) {
				fsmManager_stateIn(&gsmModule_requestServerDisconnection_state);

				softTimer_start(&timeout, 65*1000);
			}

			if(string_containsWithinLength(gsmRxDataChunk, (uint8_t *) gsmModule_response_ok, gsmRxDataChunkLen)) {
				fsmManager_gotoState(&gsmModule_requestServerDisconnection_state, __gsmModule_requestServerDisconnection_send_atSapbr2);
			}
			else if(softTimer_expired(&timeout)) {
				fsmManager_gotoState(&gsmModule_requestServerDisconnection_state, __gsmModule_requestServerDisconnection_send_atSapbr0);
			}

			if(fsmManager_isStateOut(&gsmModule_requestServerDisconnection_state)) {
				fsmManager_stateOut(&gsmModule_requestServerDisconnection_state);
			}
			break;



		case __gsmModule_requestServerDisconnection_send_atSapbr2:
			if(fsmManager_isStateIn(&gsmModule_requestServerDisconnection_state)) {
				fsmManager_stateIn(&gsmModule_requestServerDisconnection_state);

				softTimer_start(&timeout, 100);
			}

			if(softTimer_expired(&timeout)) {
				pinGsmUartTx_transmit((uint8_t *) gsmModule_command_sapbr2);
				fsmManager_gotoState(&gsmModule_requestServerDisconnection_state, __gsmModule_requestServerDisconnection_check_atSapbr2);
			}

			if(fsmManager_isStateOut(&gsmModule_requestServerDisconnection_state)) {
				fsmManager_stateOut(&gsmModule_requestServerDisconnection_state);
			}
			break;



		case __gsmModule_requestServerDisconnection_check_atSapbr2:
			if(fsmManager_isStateIn(&gsmModule_requestServerDisconnection_state)) {
				fsmManager_stateIn(&gsmModule_requestServerDisconnection_state);

				softTimer_start(&timeout, 100);
			}

			if(string_containsWithinLength(gsmRxDataChunk, (uint8_t *) "0.0.0.0", gsmRxDataChunkLen)) {
				fsmManager_gotoState(&gsmModule_requestServerDisconnection_state, __gsmModule_requestServerDisconnection_idle);

				flags_gsmModule.bits.isServerConnected = 0;
				flags_gsmModule.bits.requestServerDisconnection = 0;
			}
			else if(softTimer_expired(&timeout)) {
				fsmManager_gotoState(&gsmModule_requestServerDisconnection_state, __gsmModule_requestServerDisconnection_send_atSapbr2);
			}

			if(fsmManager_isStateOut(&gsmModule_requestServerDisconnection_state)) {
				fsmManager_stateOut(&gsmModule_requestServerDisconnection_state);
			}
			break;



		case __gsmModule_requestServerDisconnection_error:
			if(fsmManager_isStateIn(&gsmModule_requestServerDisconnection_state)) {
				fsmManager_stateIn(&gsmModule_requestServerDisconnection_state);
			}

			flags_gsmModuleError.bits.serverDisconnection = 1;
			flags_gsmModule.bits.requestServerDisconnection = 0;
			fsmManager_gotoState(&gsmModule_requestServerDisconnection_state, __gsmModule_requestServerDisconnection_idle);

			if(fsmManager_isStateOut(&gsmModule_requestServerDisconnection_state)) {
				fsmManager_stateOut(&gsmModule_requestServerDisconnection_state);
			}
			break;




		default:
			if(fsmManager_isStateIn(&gsmModule_requestServerDisconnection_state)) {
				fsmManager_stateIn(&gsmModule_requestServerDisconnection_state);
			}

			flags_gsmModule.bits.requestServerDisconnection = 0;
			fsmManager_gotoState(&gsmModule_requestServerDisconnection_state, __gsmModule_requestServerDisconnection_idle);

			if(fsmManager_isStateOut(&gsmModule_requestServerDisconnection_state)) {
				fsmManager_stateOut(&gsmModule_requestServerDisconnection_state);
			}
			break;
	}
}






//Pin Management
/*
 	PIN				IN/OUT			FUNCTION
 	--------------	--------------	--------------
	GSM_VDD			input			gpio
	GSM_PWRKEY		input			gpio
	GSM_UART_TX		output			uart
	GSM_UART_RX		input			uart
*/

static void pinGsmVdd_write(uint8_t value)
{
	HAL_GPIO_WritePin(GSM_VDD_GPIO_Port, GSM_VDD_Pin, value);
}

static void pinGsmPwrkey_write(uint8_t value)
{
	HAL_GPIO_WritePin(GSM_PWRKEY_GPIO_Port, GSM_PWRKEY_Pin, value);
}

static void pinGsmUartTx_transmit(uint8_t *gsmModule_command)
{
	__HAL_UART_DISABLE_IT(gsmHuart, UART_IT_RXNE);
	__HAL_UART_ENABLE_IT(gsmHuart, UART_IT_TC);
	HAL_UART_Transmit_IT(gsmHuart, gsmModule_command, string_length(gsmModule_command));
	__HAL_UART_DISABLE_IT(gsmHuart, UART_IT_TC);
	__HAL_UART_ENABLE_IT(gsmHuart, UART_IT_RXNE);
}

static void pinGsmUartRx_receive(void)
{
	HAL_UART_Receive_IT(gsmHuart, &gsmRxData[gsmRxPtrIn], 1);
	gsmRxPtrIn++;
	gsmRxPtrIn%=GSMRXDATA_LENGTH;
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	if(huart->Instance == gsmHuart->Instance) {
		pinGsmUartRx_receive();
	}
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{

}
