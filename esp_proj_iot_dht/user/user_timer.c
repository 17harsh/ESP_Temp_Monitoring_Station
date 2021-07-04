/*
 * user_timer.c
 *
 *  Created on: 13-Jun-2021
 *      Author: harsh
 */

#include "user_timer.h"

//system includes
#include "osapi.h"
#include "user_interface.h"

//Set-Up Debugging Macros
#ifndef ESP_TIMER_LOGGER
	#define TIMER_DEBUG(message)					do {} while(0)
	#define TIMER_DEBUG_ARGS(message, args...)		do {} while(0)
#else
	#define TIMER_DEBUG(message)					do {os_printf("[TIMER_DEBUG] %s", message); os_printf("\r\n");} while(0)
	#define TIMER_DEBUG_ARGS(message, args...)		do {os_printf("[TIMER_DEBUG] "); os_printf(message, args); os_printf("\r\n");} while(0)
#endif

// static variables
static os_timer_t osTimer1;

/******** Function Definitions ********/

/*******************************************************************************************
 * FunctionName	:  DisarmTimer1
 * Description	:  Disarms timer1.
 * Return		:  bool, true if successful,
 * 						 false if failed
 ******************************************************************************************/
bool ICACHE_FLASH_ATTR DisarmTimer1(void){
	//disarm sleep timer
	os_timer_disarm(&osTimer1);
	return true;
}

/*******************************************************************************************
 * FunctionName	:  ArmTimer1
 * Description	:  Arms timer1.
 * Parmaters	:  iTime -- time in seconds
 * 				   iRepeat -- timer repeat
 * Return		:  bool, true if successful,
 * 						 false if failed
 ******************************************************************************************/
bool ICACHE_FLASH_ATTR ArmTimer1(uint8 iTime, bool iRepeat){
	//arm sleep timer
	os_timer_arm(&osTimer1, iTime*1000, iRepeat);	//in seconds
	return true;
}

/*******************************************************************************************
 * FunctionName	:  InitTimer1
 * Description	:  Initializes timer1
 * Parameter	:  pfunction -- timer callback function
 * 				   parg -- timer callback arguments
 * Return		:  bool, true if successful,
 * 						 false if failed
 ******************************************************************************************/
bool ICACHE_FLASH_ATTR InitTimer1(void *pfunction, void *parg){
	//register timer for deep sleep
	os_timer_setfn(&osTimer1, (os_timer_func_t*) pfunction, NULL);
	return true;
}
