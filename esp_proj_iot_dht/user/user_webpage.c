/*
 * user_webpage.c
 *
 *  Created on: 08-Jun-2021
 *      Author: harsh
 */

#include "user_webpage.h"

//system includes
#include "osapi.h"

//user includes
#include "user_wifi.h"

/*******************************************************************************************
 * FunctionName	:  GetWifi_AP_HTML
 * Description	:  Used to retrieve HTML Page of Station Select Page
 * Return		:  HTML String
 ******************************************************************************************/
char* ICACHE_FLASH_ATTR GetWifi_AP_HTML(void){
	return WIFI_AP_HTML;
}

/*******************************************************************************************
 * FunctionName	:  GetWifi_AP_CSS
 * Description	:  Used to retrieve CSS of HTML Page of Station Select Page
 * Return		:  CSS String
 ******************************************************************************************/
char* ICACHE_FLASH_ATTR GetWifi_AP_CSS(void){
	return WIFI_AP_CSS;
}

/*******************************************************************************************
 * FunctionName	:  GetWifi_AP_CSS
 * Description	:  Used to retrieve JS of HTML Page of Station Select Page
 * Return		:  JS String
 ******************************************************************************************/
char* ICACHE_FLASH_ATTR GetWifi_AP_JS(void){
	return WIFI_AP_JS;
}

/*******************************************************************************************
 * FunctionName	:  UpdateJSData
 * Description	:  Appends scanned AP data in JS Script
 * Parameters	:  scanned_APs -- scanned APs array
 * 				   size -- size of scanned AP's array
 ******************************************************************************************/
void ICACHE_FLASH_ATTR UpdateJSData(struct scanned_AP_info *scanned_APs, uint8 size){
	char ssidBuff[300];
	os_memset(ssidBuff, 0, 300);
	for(uint8 i = 0; i < size; ++i){
		if(scanned_APs[i].ssid[0] != 0x00){
			if(i != 0) os_sprintf(ssidBuff + os_strlen(ssidBuff), ",");
			os_sprintf(ssidBuff + os_strlen(ssidBuff),"'%s'", (char*)scanned_APs[i].ssid);
		}
		else
			break;
	}
	char *JS_End = os_strstr(WIFI_AP_JS, "});");
	os_sprintf(JS_End + 3, "var AP=[%s];", ssidBuff);
}
