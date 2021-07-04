/*
 * user_wifi.h
 *
 *  Created on: 06-Jun-2021
 *      Author: harsh
 */

#ifndef INCLUDE_USER_WIFI_H_
#define INCLUDE_USER_WIFI_H_

//system includes
#include "c_types.h"

//uncomment for log messages
//#define ESP_WIFI_LOGGER

//Soft-AP Configuration
#define SOFTAP_SSID					"ESP8266"
#define SOFTAP_PASSWORD				"esp8266_01"
#define SOFTAP_AUTHMODE				AUTH_WPA2_PSK
#define SOFTAP_MAXCONNECTIONS		1

//Station Configuration
#define SCAN_LIST					10			//Maximum number of scanned AP's that will be stored
#define SCAN_BUTTON					5

//Wifi LEDs
#define STATION_LED					14
#define SOFTAP_LED					12
#define LOS_LED						13

//Wifi Internet ping params
#define PING_INTERNET_TASK_EVENT	4
#define GOOGLE_DNS					"8.8.8.8"
#define PING_COARSE					1
#define PING_COUNT					2

//Wifi Timers
#define STATION_TIMER				10 		//seconds
#define SOFTAP_TIMER				60		//seconds

//structure to store scanned Ap info
typedef struct scanned_AP_info{
	uint8 ssid[32];
	uint8 ssid_len;
	uint8 password[64];
	//AUTH_MODE authmode;
} AP_Info;

//API's

/*******************************************************************************************
 * FunctionName	:  InitWifi
 * Description	:  Initializes Wifi. Set wifi in Station mode and tries to reconnect using
 * 				   saved AP info in flash. If fails to do so, it switches on LOS LED
 * Parameter	:  Timer_cb -- timer callback function.
 * Return		:  bool, true if successful,
 * 						 false if failed
 ******************************************************************************************/
bool ICACHE_FLASH_ATTR InitWifi(void* Timer_cb);

/*******************************************************************************************
 * FunctionName	:  ConnectToStation
 * Description	:  Connects to Wifi Router(AP)
 * Parameters	:  iData -- Raw HTTP Data received consisting of ssid and password
 * 				   iDataLength -- iData length
 * Return		:  bool, true if successful,
 * 						 false if failed
 ******************************************************************************************/
bool ICACHE_FLASH_ATTR ConnectToStation(char *iData, uint16 iDataLength);

/***************************************************************************************
 * FunctionName	:  ConnectedToInternet
 * Description	:  Checks whether Esp is connected to internet or not
 * Returns		:  bool, true if connected
 * 						 false if not connected
 **************************************************************************************/
bool ICACHE_FLASH_ATTR ConnectedToInternet(void);

#endif /* INCLUDE_USER_WIFI_H_ */
