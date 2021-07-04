/*
 * user_espconn.h
 *
 *  Created on: 07-Jun-2021
 *      Author: harsh
 */

#ifndef INCLUDE_USER_ESPCONN_H_
#define INCLUDE_USER_ESPCONN_H_

#include "c_types.h"

//uncomment for log messages
#define ESP_ESPCONN_LOGGER

#define TCP_LOCAL_PORT		80

// APIs

/*******************************************************************************************
 * FunctionName	:  InitWebServer
 * Description	:  Initializes ESP Web Server
 * Return		:  0 if successful, else failed
 ******************************************************************************************/
sint8 ICACHE_FLASH_ATTR InitESPConn(void);

/*******************************************************************************************
 * FunctionName	:  StartWebServer
 * Description	:  Starts the Web Server
 * Return		:  0 if successful, else failed
 ******************************************************************************************/
sint8 ICACHE_FLASH_ATTR StartLocalServer(void);

/*******************************************************************************************
 * FunctionName	:  StopWebServer
 * Description	:  Stops the Web Server
 * Return		:  0 if successful, else failed
 ******************************************************************************************/
sint8 ICACHE_FLASH_ATTR StopLocalServer(void);

/*******************************************************************************************
 * FunctionName	:  SendDataToRemoteServer
 * Description	:
 * Return		:
 ******************************************************************************************/
sint8 ICACHE_FLASH_ATTR SendDataToRemoteServer(char *iUrl, uint16 iUrlLength, char*iData, uint16 iDataLength);

#endif /* INCLUDE_USER_ESPCONN_H_ */
