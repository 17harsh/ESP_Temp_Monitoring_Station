/*
 * user_config.h
 *
 *  Created on: 30-Mar-2021
 *      Author: harsh
 */

#ifndef INCLUDE_USER_CONFIG_H_
#define INCLUDE_USER_CONFIG_H_

//uncomment for Debug Log
#define ESP_LOG

//Set-Up Debugging Macros
#ifndef ESP_LOG
	#define ESP_DEBUG(message)					do {} while(0)
	#define ESP_DEBUG_ARGS(message, args...)	do {} while(0)
#else
	#define ESP_DEBUG(message)					do {os_printf("[ESP-DEBUG] %s", message); os_printf("\r\n");} while(0)
	#define ESP_DEBUG_ARGS(message, args...)	do {os_printf("[ESP-DEBUG] "); os_printf(message, args); os_printf("\r\n");} while(0)
#endif

//dht config
#define DHT_PIN					4

#endif /* INCLUDE_USER_CONFIG_H_ */



