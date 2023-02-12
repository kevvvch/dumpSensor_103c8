
#ifndef INC_GSMMODULEATCOMMANDS_H_
#define INC_GSMMODULEATCOMMANDS_H_

/* AT COMMANDS */

const uint8_t gsmModule_command_at[] 					= "AT\r\n";

const uint8_t gsmModule_command_cpowd1[] 				= "AT+CPOWD=1\r\n";

const uint8_t gsmModule_command_cgpspwr1[] 				= "AT+CGPSPWR=1\r\n";
const uint8_t gsmModule_command_cgpsrst0[] 				= "AT+CGPSRST=0\r\n";
const uint8_t gsmModule_command_cgpsrst1[] 				= "AT+CGPSRST=1\r\n";
const uint8_t gsmModule_command_cgpsstatus[] 			= "AT+CGPSSTATUS?\r\n";
const uint8_t gsmModule_command_cgpsinf2[] 				= "AT+CGPSINF=2\r\n";

const uint8_t gsmModule_command_atcmee1[]				= "AT+CMEE=1\r\n";
const uint8_t gsmModule_command_cfun0[] 				= "AT+CFUN=0\r\n";
const uint8_t gsmModule_command_cfun1[] 				= "AT+CFUN=1\r\n";
const uint8_t gsmModule_command_cpin[] 					= "AT+CPIN?\r\n";
const uint8_t gsmModule_command_cband_allBand[]			= "AT+CBAND=\"ALL_BAND\"\r\n";
const uint8_t gsmModule_command_creg1[]					= "AT+CREG=1\r\n";
const uint8_t gsmModule_command_creg[]					= "AT+CREG?\r\n";
const uint8_t gsmModule_command_sapbr3contype[] 		= "AT+SAPBR=3,1,\"Contype\",\"GPRS\"\r\n";
const uint8_t gsmModule_command_sapbr3apnPersonal[] 	= "AT+SAPBR=3,1,\"APN\",\"gprs.personal.com\"\r\n";
const uint8_t gsmModule_command_sapbr3userPersonal[] 	= "AT+SAPBR=3,1,\"USER\",\"gprs\"\r\n";
const uint8_t gsmModule_command_sapbr3pwdPersonal[] 	= "AT+SAPBR=3,1,\"PWD\",\"gprs\"\r\n";
const uint8_t gsmModule_command_sapbr1[] 				= "AT+SAPBR=1,1\r\n";
const uint8_t gsmModule_command_sapbr2[] 				= "AT+SAPBR=2,1\r\n";
const uint8_t gsmModule_command_cops[] 					= "AT+COPS?\r\n";
const uint8_t gsmModule_command_httpinit[]				= "AT+HTTPINIT\r\n";
const uint8_t gsmModule_command_httppara_cid[] 			= "AT+HTTPPARA=\"CID\",1\r\n";
const uint8_t gsmModule_command_httppara_url[] 			= "AT+HTTPPARA=\"URL\",\"34.170.81.68:80/index.php?";	//Se completa con datos de los sensores
const uint8_t gsmModule_command_httpaction1[] 			= "AT+HTTPACTION=1\r\n";
const uint8_t gsmModule_command_sapbr0[] 				= "AT+SAPBR=0,1\r\n";
const uint8_t gsmModule_command_httpterm[] 				= "AT+HTTPTERM\r\n";
const uint8_t gsmModule_command_[] = "AT+\r\n";



const uint8_t gsmModule_response_ok[]					= "OK";
const uint8_t gsmModule_response_error[]				= "ERROR";
const uint8_t gsmModule_response_cme_error[]			= "+CME ERROR:";
const uint8_t gsmModule_response_normal_powerdown[]		= "NORMAL POWER DOWN";
const uint8_t gsmModule_response_cpinNotReady[]			= "+CPIN: NOT READY";
const uint8_t gsmModule_response_cpinNotInserted[]		= "+CPIN: NOT INSERTED";
const uint8_t gsmModule_response_cpinSimPin[]			= "+CPIN: SIM PIN";
const uint8_t gsmModule_response_cpinReady[]			= "+CPIN: READY";
const uint8_t gsmModule_response_creg1_0[]				= "+CREG: 1,0";
const uint8_t gsmModule_response_creg1_1[]				= "+CREG: 1,1";
const uint8_t gsmModule_response_creg1_2[]				= "+CREG: 1,2";
const uint8_t gsmModule_response_cops_0[]				= "+COPS: 0";
const uint8_t gsmModule_response_sapbr_1_1[]			= "+SAPBR: 1,1,";
const uint8_t gsmModule_response_httpaction1_200[] 		= "1,200,";
const uint8_t gsmModule_response_httpaction[] 			= "+HTTPACTION:";
const uint8_t gsmModule_response_gps2dFix[]				= "2D Fix";
const uint8_t gsmModule_response_gps3dFix[]				= "3D Fix";
const uint8_t gsmModule_response_gpsInf2[]				= "+CGPSINF: 2,";

const uint8_t gsmModule_unsolicited_creg[]				= "+CREG";
const uint8_t gsmModule_unsolicited_creg0[]				= ": 0";
const uint8_t gsmModule_unsolicited_creg1[]				= ": 1";
const uint8_t gsmModule_unsolicited_creg2[]				= ": 2";



#endif /* INC_GSMMODULEATCOMMANDS_H_ */
