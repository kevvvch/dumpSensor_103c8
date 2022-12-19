/*
 * gsmModuleManager.h
 *
 *  Created on: Aug 27, 2022
 *      Author: kevin
 */

#ifndef INC_GSMMODULEMANAGER_H_
#define INC_GSMMODULEMANAGER_H_

#include "main.h"






/* PUBLIC FUNCTIONS */
void gsmModule_powerOn(void);
void gsmModule_powerOff(void);
void gsmModule_handler(void);
void gsmModule_init(UART_HandleTypeDef *huart);

void gsmModule_pwrkeyOn(void);
void gsmModule_pwrkeyOff(void);
void gsmModule_gpsOn(void);
void gsmModule_gpsInfo(void);
void gsmModule_gpsOff(void);
void gsmModule_serverConnect(void);
void gsmModule_serverDataSend(uint8_t *data);
void gsmModule_serverDataReceive(void);
void gsmModule_serverDisconnect(void);

uint8_t gsmModule_requestedPowerOn(void);
uint8_t gsmModule_requestedPowerOff(void);
uint8_t gsmModule_requestedGpsOn(void);
uint8_t gsmModule_requestedGpsInfo(void);
uint8_t gsmModule_requestedGpsOff(void);
uint8_t gsmModule_requestedServerConnection(void);
uint8_t gsmModule_requestedServerDataSend(void);
uint8_t gsmModule_requestedServerDataReceive(void);
uint8_t gsmModule_requestedServerDisconnection(void);

uint8_t gsmModule_errorServerSendData(void);

uint8_t gsmModule_isPowered(void);
uint8_t gsmModule_isGpsOn(void);
uint8_t gsmModule_isGpsFixed(void);
uint8_t gsmModule_isServerConnected(void);
uint8_t gsmModule_isServerDataSent(void);
uint8_t gsmModule_isServerDataReceived(void);


#endif /* INC_GSMMODULEMANAGER_H_ */
