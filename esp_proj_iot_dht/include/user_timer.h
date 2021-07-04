/*
 * user_timer.h
 *
 *  Created on: 13-Jun-2021
 *      Author: harsh
 */

#ifndef INCLUDE_USER_TIMER_H_
#define INCLUDE_USER_TIMER_H_

#include "c_types.h"

//uncomment for Debug Log
//#define ESP_TIMER_LOGGER

// API's
/*******************************************************************************************
 * FunctionName	:  DisarmTimer1
 * Description	:  Disarms timer1.
 * Return		:  bool, true if successful,
 * 						 false if failed
 ******************************************************************************************/
bool ICACHE_FLASH_ATTR DisarmTimer1(void);

/*******************************************************************************************
 * FunctionName	:  ArmTimer1
 * Description	:  Arms timer1.
 * Parmaters	:  iTime -- time in seconds
 * 				   iRepeat -- timer repeat
 * Return		:  bool, true if successful,
 * 						 false if failed
 ******************************************************************************************/
bool ICACHE_FLASH_ATTR ArmTimer1(uint8 iTime, bool iRepeat);

/*******************************************************************************************
 * FunctionName	:  InitTimer1
 * Description	:  Initializes timer1
 * Parameter	:  pfunction -- timer callback function
 * 				   parg -- timer callback arguments
 * Return		:  bool, true if successful,
 * 						 false if failed
 ******************************************************************************************/
bool ICACHE_FLASH_ATTR InitTimer1(void *pfunction, void *parg);

#endif /* INCLUDE_USER_TIMER_H_ */
