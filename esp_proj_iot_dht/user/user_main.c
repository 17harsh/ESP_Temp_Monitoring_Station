/*
 * user_main.c
 *
 *  Created on: 30-Mar-2021
 *      Author: harsh
 */

#include "osapi.h"
#include "user_interface.h"
#include "espconn.h"
#include "mem.h"

//driver libs
#include "driver/dht.h"
#include "driver/http.h"
#include "driver/uart.h"

//user includes
#include "user_config.h"
#include "user_espconn.h"
#include "user_wifi.h"
#include "user_timer.h"

//UART
#define UART_BAUD								115200

/***********************************************************************************************************************************************************************/
/***********************************************************************************************************************************************************************/
//Flash Size MACRO
#define SPI_FLASH_SIZE_MAP						4

/* user must define this partition */
/* mandatory */
#define SYSTEM_PARTITION_RF_CAL_SZ                0x1000	// 4KB
#define SYSTEM_PARTITION_PHY_DATA_SZ              0x1000	// 4KB
#define SYSTEM_PARTITION_SYSTEM_PARAMETER_SZ      0x3000	// 12KB

#define SPI_FLASH_SIZE							0x400000	// 4MB

#define SYSTEM_PARTITION_RF_CAL_ADDR                SPI_FLASH_SIZE - SYSTEM_PARTITION_SYSTEM_PARAMETER_SZ - SYSTEM_PARTITION_PHY_DATA_SZ - SYSTEM_PARTITION_RF_CAL_SZ
#define SYSTEM_PARTITION_PHY_DATA_ADDR              SPI_FLASH_SIZE - SYSTEM_PARTITION_SYSTEM_PARAMETER_SZ - SYSTEM_PARTITION_PHY_DATA_SZ
#define SYSTEM_PARTITION_SYSTEM_PARAMETER_ADDR      SPI_FLASH_SIZE - SYSTEM_PARTITION_SYSTEM_PARAMETER_SZ


static const partition_item_t at_partition_table[] = {
		{SYSTEM_PARTITION_RF_CAL, SYSTEM_PARTITION_RF_CAL_ADDR, SYSTEM_PARTITION_RF_CAL_SZ},
		{SYSTEM_PARTITION_PHY_DATA, SYSTEM_PARTITION_PHY_DATA_ADDR, SYSTEM_PARTITION_PHY_DATA_SZ},
		{SYSTEM_PARTITION_SYSTEM_PARAMETER, SYSTEM_PARTITION_SYSTEM_PARAMETER_ADDR, SYSTEM_PARTITION_SYSTEM_PARAMETER_SZ}
};
/***********************************************************************************************************************************************************************/
/***********************************************************************************************************************************************************************/

#define	MAIN_TIMER_DURATION			10	//in seconds

void ICACHE_FLASH_ATTR _ReadTempAndUpload(void){

	float humidity = 0.0, temperature = 0.0;
	uint8 tempUnit = Celcius;
	if(ConnectedToInternet() && DHT_OK == dht_read(&humidity, &temperature, tempUnit)){
		//convert float to integers because apparently this shit can't handle float to string -_-
		int32_t humidity_i = humidity;
		int32_t humidity_d = (humidity-humidity_i)*10;
		int32_t temperature_i = temperature;
		int32_t temperature_d = (temperature-temperature_i)*10;
		ESP_DEBUG_ARGS("Humidity : %d.%d %% and Temperature : %d.%d C", humidity_i, humidity_d, temperature_i, temperature_d);

		//create json of data
		char jsonData[65] = {0};
		os_memset(jsonData, 0, 65);
		os_sprintf(jsonData, "{ 'Humidity' : %d.%d, 'Temperature' : %d.%d, 'Unit' : %d }", humidity_i, humidity_d, temperature_i, temperature_d, tempUnit);
		ESP_DEBUG_ARGS("content : %s", jsonData);

		SendDataToRemoteServer("esp8266.com", 3, jsonData, os_strlen(jsonData));

		//sendtoserver and sleep
		//system_deep_sleep(10*1000*1000);
	}
	else{
		//system_deep_sleep(10*1000*1000);
	}
}

void ICACHE_FLASH_ATTR InitUART(void){
	/**** Initializing UART BAUD ****/
	uart_init(UART_BAUD, UART_BAUD);
	os_delay_us(500);

	//clear screen and moves Cursor to left
	os_printf("\033[2J");
	ESP_DEBUG("ESP8266, User-Init");
}

void ICACHE_FLASH_ATTR espUserInit(void){
	/**** Initialize UART for logging ****/
	InitUART();

	/**** Init webserver ****/
	ESP_DEBUG("Initializing ESP Conn");
	InitESPConn();

	/**** Init Wifi ****/
	ESP_DEBUG("Initializing Wifi");
	InitWifi(_ReadTempAndUpload);

	/**** Init DHT ****/
	ESP_DEBUG("Initializing DHT");
	dht_init(DHT_PIN);
}

void ICACHE_FLASH_ATTR user_pre_init(void)
{
    if(!system_partition_table_regist(at_partition_table, sizeof(at_partition_table)/sizeof(at_partition_table[0]),SPI_FLASH_SIZE_MAP)) {
		os_printf("system_partition_table_regist fail\r\n");
		while(1);
	}
}

void ICACHE_FLASH_ATTR user_init(void)
{
	system_init_done_cb(espUserInit);
}

