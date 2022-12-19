/*
 * powerModeManager.h
 *
 *  Created on: May 16, 2022
 *      Author: kevin
 */

#ifndef INC_POWERMODEMANAGER_H_
#define INC_POWERMODEMANAGER_H_

#include "main.h"


void powerMode_init(RTC_HandleTypeDef* hrtc);
void powerMode_enterStandbyMode(uint32_t sec);

#endif /* INC_POWERMODEMANAGER_H_ */
