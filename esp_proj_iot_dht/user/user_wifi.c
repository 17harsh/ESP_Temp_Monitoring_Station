/*
 * user_wifi.c
 *
 *  Created on: 06-Jun-2021
 *      Author: harsh
 */

//system includes
#include "osapi.h"
#include "user_interface.h"
#include "ping.h"

//user includes
#include "user_wifi.h"
#include "user_espconn.h"
#include "user_webpage.h"
#include "user_timer.h"

//Set-Up Debugging Macros
#ifndef ESP_WIFI_LOGGER
	#define WIFI_DEBUG(message)					do {} while(0)
	#define WIFI_DEBUG_ARGS(message, args...)	do {} while(0)
#else
	#define WIFI_DEBUG(message)					do {os_printf("[WIFI-DEBUG] %s", message); os_printf("\r\n");} while(0)
	#define WIFI_DEBUG_ARGS(message, args...)	do {os_printf("[WIFI-DEBUG] "); os_printf(message, args); os_printf("\r\n");} while(0)
#endif

#define WIFI_ASSERT_AND_RET(ret, value)			if(ret != value) return ret;

/******************* GPIO_PIN PARAMETERS *******************/

#define NUMBER_VALID_GPIOS	12
static const uint8_t gpio_num[NUMBER_VALID_GPIOS] = {0,1,2,3,4,5,9,10,12,13,14,15};
static const uint32_t gpio_mux[NUMBER_VALID_GPIOS] = {PERIPHS_IO_MUX_GPIO0_U, PERIPHS_IO_MUX_U0TXD_U,PERIPHS_IO_MUX_GPIO2_U, PERIPHS_IO_MUX_U0RXD_U, PERIPHS_IO_MUX_GPIO4_U, PERIPHS_IO_MUX_GPIO5_U,
										PERIPHS_IO_MUX_SD_DATA2_U, PERIPHS_IO_MUX_SD_DATA3_U, PERIPHS_IO_MUX_MTDI_U, PERIPHS_IO_MUX_MTCK_U, PERIPHS_IO_MUX_MTMS_U, PERIPHS_IO_MUX_MTDO_U};
static const uint8_t gpio_func[NUMBER_VALID_GPIOS] = {FUNC_GPIO0, FUNC_GPIO1, FUNC_GPIO2, FUNC_GPIO3, FUNC_GPIO4, FUNC_GPIO5, FUNC_GPIO9, FUNC_GPIO10, FUNC_GPIO12, FUNC_GPIO13, FUNC_GPIO14, FUNC_GPIO15};

/**********************************************************/

// user task queue
static os_event_t taskQueue[3] = {0};

// placeholder to save scanned AP's info and placeholder pointing to AP to connect
static AP_Info scanned_APs[SCAN_LIST] = {0};
static AP_Info AP;

//ping params
struct ping_option ping_option;
static uint8 pingNo = 1;
static uint32 total_payload = 0;

//LOS Status
static bool LOS = true;

static bool scanButtonPressed = false;

/******** Function Definitions ********/

/***************************************************************************************
 * FunctionName	:  _SetLOS
 * Description	:  Set the state of LOS LED and saves it in global param
 * Parameter	:  iValue -- value to be set
 **************************************************************************************/
void ICACHE_FLASH_ATTR _SetLOS(bool iValue){
	GPIO_OUTPUT_SET(GPIO_ID_PIN(LOS_LED), iValue);
	LOS = iValue;
}

/***************************************************************************************
 * FunctionName	:  _wifiEventHandler
 * Description	:  wifi event handler funcntion
 * Parameters	:  event -- wifi event type input parameter
 **************************************************************************************/
void ICACHE_FLASH_ATTR _wifiEventHandler(System_Event_t *event){
	WIFI_DEBUG_ARGS("wifi event occurred : %d", event->event);

	bool ret = false;
	switch (event->event) {
	case EVENT_STAMODE_CONNECTED:
		WIFI_DEBUG_ARGS("Connected to ssid %s, channel %d",event->event_info.connected.ssid, event->event_info.connected.channel);

		// Switch ON Station LED
		GPIO_OUTPUT_SET(GPIO_ID_PIN(STATION_LED), 1);

		break;
	case EVENT_STAMODE_DISCONNECTED:
		WIFI_DEBUG_ARGS("Disconnected from ssid %s, reason %d",event->event_info.disconnected.ssid, event->event_info.disconnected.reason);

		// Switch OFF STATION LED and ON LOS LED
		GPIO_OUTPUT_SET(GPIO_ID_PIN(STATION_LED), 0);
		_SetLOS(1);

		break;
	case EVENT_STAMODE_AUTHMODE_CHANGE:
		WIFI_DEBUG_ARGS("Authmode changed from %d to %d",event->event_info.auth_change.old_mode, event->event_info.auth_change.new_mode);
		break;
	case EVENT_STAMODE_GOT_IP:
		WIFI_DEBUG_ARGS("Got IP, ip:" IPSTR ",mask:" IPSTR ",gw:" IPSTR,
		IP2STR(&event->event_info.got_ip.ip),
		IP2STR(&event->event_info.got_ip.mask),
		IP2STR(&event->event_info.got_ip.gw));

		//PING Internet and Handle LOS LED
		ret = system_os_post(USER_TASK_PRIO_2, PING_INTERNET_TASK_EVENT, 0);
		WIFI_DEBUG_ARGS("call user task to ping internet, ret : %d", ret);

		break;
	case EVENT_STAMODE_DHCP_TIMEOUT:
		WIFI_DEBUG("DHCP Timeout occurred");
		break;
	case EVENT_SOFTAPMODE_STACONNECTED:
		WIFI_DEBUG_ARGS("Station mac: " MACSTR "join, AID = %d",
		MAC2STR(event->event_info.sta_connected.mac),
		event->event_info.sta_connected.aid);
		break;
	case EVENT_SOFTAPMODE_STADISCONNECTED:
		WIFI_DEBUG_ARGS("Station mac: " MACSTR "left, AID = %d",
		MAC2STR(event->event_info.sta_disconnected.mac),
		event->event_info.sta_disconnected.aid);
		break;
	case EVENT_SOFTAPMODE_PROBEREQRECVED:
		WIFI_DEBUG_ARGS("Probe, rssi: %d , mac: " MACSTR,
		event->event_info.ap_probereqrecved.rssi,
		MAC2STR(event->event_info.ap_probereqrecved.mac));
		break;
	case EVENT_OPMODE_CHANGED:
		WIFI_DEBUG_ARGS("Operation mode changed from %d to %d", event->event_info.opmode_changed.old_opmode, event->event_info.opmode_changed.new_opmode);
		if(event->event_info.opmode_changed.new_opmode == STATION_MODE){

			// Switch Switch ON STATION LED, Switch OFF SOFTAP LED and Switch ON LOS LED
			GPIO_OUTPUT_SET(GPIO_ID_PIN(STATION_LED), 1);
			GPIO_OUTPUT_SET(GPIO_ID_PIN(SOFTAP_LED), 0);
			_SetLOS(1);

			// stop esp8266 webserver
			WIFI_DEBUG("Stopping WebServer");
			StopLocalServer();

			//arm timer
			DisarmTimer1();
			ArmTimer1(STATION_TIMER, false);
		}
		else if(event->event_info.opmode_changed.new_opmode == SOFTAP_MODE){

			// Switch Switch OFF STATION LED, Switch ON SOFTAP LED and Switch ON LOS LED
			GPIO_OUTPUT_SET(GPIO_ID_PIN(STATION_LED), 0);
			GPIO_OUTPUT_SET(GPIO_ID_PIN(SOFTAP_LED), 1);
			_SetLOS(1);

			//start esp8266 webserver
			WIFI_DEBUG("Starting WebServer");
			StartLocalServer();

			//arm timer
			DisarmTimer1();
			ArmTimer1(SOFTAP_TIMER, false);
		}
		break;
	case EVENT_SOFTAPMODE_DISTRIBUTE_STA_IP:
		WIFI_DEBUG_ARGS("Distribute sta ip, mac: " MACSTR ",ip: " IPSTR " ,AID = %d",
		MAC2STR(event->event_info.distribute_sta_ip.mac),
		IP2STR(&event->event_info.distribute_sta_ip.ip),
		event->event_info.distribute_sta_ip.aid);
		break;
	case EVENT_MAX:
		WIFI_DEBUG("Max Events occurred");
		break;
	default:
		break;
	}
}

/***************************************************************************************
 * FunctionName	:  _Ping_recv_cb
 * Description	:  callback function for receiving ping response.
 * Paramters	:  arg -- ping options
 *				   pdata -- ping response
 **************************************************************************************/
void ICACHE_FLASH_ATTR _Ping_recv_cb (void* arg, void *pdata){
	WIFI_DEBUG("Ping response recieve");

	//read ping response
	struct ping_resp *ping_resp = (struct ping_resp*) pdata;
	if(ping_resp != NULL){
		uint32 data_bytes = ping_resp->bytes;
		sint8 ping_error = ping_resp->ping_err;
		if(ping_error == 0){
			total_payload += data_bytes;
		}
	}
	WIFI_DEBUG_ARGS("Ping total_payload : %d and ping no : %d", total_payload, pingNo);
	if(pingNo >= PING_COUNT){
		if(total_payload >= (32 * PING_COUNT)){
			// Switch OFF LOS LED
			_SetLOS(0);
		}
		else{
			// Switch ON LOS LED
			GPIO_OUTPUT_SET(GPIO_ID_PIN(LOS_LED), 1);
		}
		pingNo = 0;
		total_payload = 0;
	}

	pingNo++;
}

/***************************************************************************************
 * FunctionName	:  _PingInternet
 * Description	:  Pings Google DNS (8.8.8.8) to check internet connectivity.
 * Returns		:  bool, true if Successful
 * 						 false if failed
 **************************************************************************************/
bool ICACHE_FLASH_ATTR _PingInternet(void){
	WIFI_DEBUG("Pinging google DNS at 8.8.8.8 ");

	bool ret = false;

	ping_option.count = PING_COUNT;
	ping_option.ip = ipaddr_addr(GOOGLE_DNS);
	ping_option.coarse_time = PING_COARSE;
	ping_option.sent_function = NULL;

	//Register ping recv function
	ret = ping_regist_recv(&ping_option, _Ping_recv_cb);
	WIFI_DEBUG_ARGS("register ping recv callback, ret : %d", ret);

	ret = ping_start(&ping_option);
	WIFI_DEBUG_ARGS("Pinging google DNS at 8.8.8.8, ret: %d", ret);

	return ret;
}

/***************************************************************************************
 * FunctionName	:  _Scan_done_cb
 * Description	:  callback function for wifi available AP scan.
 *				   Scans Available APs and stores it in a placeholder [array] as well as
 *				   update JS object
 * Parameters	:  arg -- list of scanned APs (bss_info)
 * 				   status -- scan status
 **************************************************************************************/
void ICACHE_FLASH_ATTR _Scan_done_cb(void *arg, STATUS status){
	WIFI_DEBUG_ARGS("inside scan done cb, status : %d", status);
	if(status == OK){
		struct bss_info *bss_link = (struct bss_info *)arg;
		os_memset(scanned_APs, 0, SCAN_LIST*sizeof(AP_Info));
		for(uint8 i = 0; i < SCAN_LIST; i++){
			if(bss_link != NULL){
				WIFI_DEBUG_ARGS("AP ssid : %s, ssid length : %d", bss_link->ssid, bss_link->ssid_len);

				os_memcpy(scanned_APs[i].ssid, bss_link->ssid, bss_link->ssid_len);
				scanned_APs[i].ssid_len = bss_link->ssid_len;
				//scanned_APs[i].authmode = bss_link->authmode;

				bss_link = STAILQ_NEXT(bss_link, next);
			}
			else break;
		}
		//modify scan AP js
		UpdateJSData(scanned_APs, SCAN_LIST);

		//set wifi in Soft-AP mode
		bool ret = false;
		ret = system_os_post(USER_TASK_PRIO_2, SOFTAP_MODE, 0);
		WIFI_DEBUG_ARGS("call user task to change op mode to SoftAP, ret : %d", ret);
	}
}

/***************************************************************************************
 * FunctionName	:  _WifiUserTasks
 * Description	:  User Task callback function for changing wifi operation mode
 * 				   and pinging google DNS for checking internet connectivity.
 * Parameters	:  event -- event type input parameter
 **************************************************************************************/
void ICACHE_FLASH_ATTR _WifiUserTasks(os_event_t *event){
	WIFI_DEBUG_ARGS("Inside Wifi user task, event : %d", event->sig);

	bool ret = false;
	bool doScan = event->par;
	switch (event->sig) {
	case STATION_MODE:
		//set wifi to station mode
		ret = wifi_set_opmode_current(STATION_MODE);
		WIFI_DEBUG_ARGS("set current opmode, ret : %d", ret);

		struct station_config station_config;
		ret = wifi_station_get_config_default(&station_config);
		WIFI_DEBUG_ARGS("get station flash config, ret : %d", ret);

		if(!doScan){
			os_memset(station_config.ssid, 0, sizeof(station_config.ssid));
			os_memcpy(station_config.ssid, AP.ssid, sizeof(AP.ssid));
			WIFI_DEBUG_ARGS("wifi Station ssid : %s", station_config.ssid);

			os_memset(station_config.password, 0, sizeof(station_config.password));
			os_memcpy(station_config.password, AP.password, sizeof(AP.password));
			WIFI_DEBUG_ARGS("wifi Station password : %s", station_config.password);

			//ret = wifi_station_set_config_current(&station_config);
			ret = wifi_station_set_config(&station_config);
			WIFI_DEBUG_ARGS("set Station config, ret : %d", ret);

			ret = wifi_station_connect();
			WIFI_DEBUG_ARGS("Connect to Station, ret : %d", ret);
		}
		else{
			//scan wifi networks (AP's)
			ret = wifi_station_scan(NULL, _Scan_done_cb);
			WIFI_DEBUG_ARGS("station scan, ret : %d", ret);
		}
		break;
	case SOFTAP_MODE:
		//set wifi to soft ap mode
		ret = wifi_set_opmode_current(SOFTAP_MODE);
		WIFI_DEBUG_ARGS("set current opmode, ret : %d", ret);

		struct softap_config softAP_config;
		ret = wifi_softap_get_config(&softAP_config);
		WIFI_DEBUG_ARGS("get station config, ret : %d", ret);

		//set Soft AP mode configuration
		if(os_strcmp(softAP_config.ssid, SOFTAP_SSID) != 0 &&
			os_strcmp(softAP_config.password, SOFTAP_PASSWORD) != 0){

			softAP_config.authmode = SOFTAP_AUTHMODE;
			softAP_config.max_connection = SOFTAP_MAXCONNECTIONS;

			os_memset(softAP_config.ssid, 0, sizeof(softAP_config.ssid));
			os_memcpy(softAP_config.ssid, SOFTAP_SSID, os_strlen(SOFTAP_SSID));
			softAP_config.ssid_len = os_strlen(SOFTAP_SSID);
			WIFI_DEBUG_ARGS("SoftAP ssid : %s", softAP_config.ssid);

			os_memset(softAP_config.password, 0, sizeof(softAP_config.password));
			os_memcpy(softAP_config.password, SOFTAP_PASSWORD, os_strlen(SOFTAP_PASSWORD));
			WIFI_DEBUG_ARGS("SoftAP password : %s", softAP_config.password);

			ret = wifi_softap_set_config(&softAP_config);
			WIFI_DEBUG_ARGS("set SoftAP config, ret : %d", ret);
		}
		break;
	case PING_INTERNET_TASK_EVENT:
		//Ping Google DNS for checking internet connectivity
		ret = _PingInternet();
		WIFI_DEBUG_ARGS("ping internet, ret : %d", ret);
		break;
	default:
		break;
	}
}

/*******************************************************************************************
 * FunctionName	:  _InitGPIO
 * Description	:  Initializes GPIO.
 * Parameters	:  iPin -- GPIO PIN
 * 				:  iAsOutput -- true, if output; false, if input
 * 				:  iEnablePullUp -- true, if high; false, if low
 * Return		:  bool, true if successful,
 * 						 false if failed
 ******************************************************************************************/
bool ICACHE_FLASH_ATTR _InitGPIO(uint8 iPin, bool iAsOutput, bool iEnablePullUp){
	uint8 iGPIO_Pin = iPin;
	int8 _pin = -1;
	//get pin
	for(uint8_t index = 0; index < NUMBER_VALID_GPIOS; ++index){
		if(gpio_num[index] == iGPIO_Pin){
			_pin = index;
			WIFI_DEBUG_ARGS("%d will be configured as GPIO", iGPIO_Pin);
			break;
		}
	}
	if (_pin == -1){
		WIFI_DEBUG_ARGS("%d cannot be configured as GPIO", iGPIO_Pin);
		return false;
	}
	//set GPIO Function Selection Register
	PIN_FUNC_SELECT(gpio_mux[_pin], gpio_func[_pin]);
	WIFI_DEBUG("Pin function select register is set");

	if(!iAsOutput){
		//set pin as input
		GPIO_DIS_OUTPUT(GPIO_ID_PIN(gpio_num[_pin]));
		if(iEnablePullUp)
			PIN_PULLUP_EN(gpio_mux[_pin]);
		else
			PIN_PULLUP_DIS(gpio_mux[_pin]);
	}
	else{
		//set pin as output
		GPIO_OUTPUT_SET(GPIO_ID_PIN(gpio_num[_pin]), iEnablePullUp);
	}
	return true;
}

/*******************************************************************************************
 * FunctionName	:  _InitWifiStatusLEDs
 * Description	:  Initializes GPIO for STATION, SOFT_AP and LOS LEDs
 * Return		:  bool, true if successful,
 * 						 false if failed
 ******************************************************************************************/
bool ICACHE_FLASH_ATTR _InitWifiStatusLEDs(){
	WIFI_DEBUG("Initializing Wifi LED GPIO.");

	bool ret = false;
	//init Station LED
	ret = _InitGPIO(STATION_LED, 1, 0);
	//init SoftAP LED
	ret = _InitGPIO(SOFTAP_LED, 1, 0);
	//init LOS LED
	ret = _InitGPIO(LOS_LED, 1, 0);
	return ret;
}

/*******************************************************************************************
 * FunctionName	:  _InterruptHandler
 * Description	:  Scan Button ISR. Puts the wifi in Station Mode and then scan AP's
 ******************************************************************************************/
void _InterruptHandler(){
	WIFI_DEBUG("Inside ISR");

	uint32 gpio_status;
	gpio_status = GPIO_REG_READ(GPIO_STATUS_ADDRESS);
	GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, gpio_status);

	uint8 iGPIO_Pin = SCAN_BUTTON;
	if(!GPIO_INPUT_GET(iGPIO_Pin)){
		scanButtonPressed = true;

		//Disable esp8266 timer1
		DisarmTimer1();

		//set wifi in Station mode and force it to scan
		bool ret = false;
		ret = system_os_post(USER_TASK_PRIO_2, STATION_MODE, 1);
	}
}

/*******************************************************************************************
 * FunctionName	:  _InitScanButton
 * Description	:  Initializes GPIO and registers interrupt for the Scan Station Button
 * Return		:  bool, true if successful,
 * 						 false if failed
 ******************************************************************************************/
bool ICACHE_FLASH_ATTR _InitScanButton(){
	WIFI_DEBUG("Initializing Scan GPIO.");
	scanButtonPressed = false;

	bool ret = false;
	ret = _InitGPIO(SCAN_BUTTON, 0, 1);
	if(ret){
		//disable all interrupts
		ETS_GPIO_INTR_DISABLE();
		//setup interrupt handler function
		ETS_GPIO_INTR_ATTACH(_InterruptHandler,NULL);
		//set interrupt state
		gpio_pin_intr_state_set(GPIO_ID_PIN(SCAN_BUTTON),GPIO_PIN_INTR_NEGEDGE);
		//enable interrupt
		ETS_GPIO_INTR_ENABLE();
	}
	return ret;
}

/*******************************************************************************************
 * FunctionName	:  InitWifi
 * Description	:  Initializes Wifi. Set wifi in Station mode and tries to reconnect using
 * 				   saved AP info in flash. If fails to do so, it switches on LOS LED
 * Return		:  bool, true if successful,
 * 						 false if failed
 ******************************************************************************************/
bool ICACHE_FLASH_ATTR InitWifi(void* Timer_cb){
	bool ret = false;

	// Init Station and SoftAP Timer
	ret = InitTimer1(Timer_cb, NULL);
	WIFI_DEBUG_ARGS("Initialized Timer, ret : %d", ret);
	DisarmTimer1();

	//Initialize scan button
	ret = _InitScanButton();
	WIFI_ASSERT_AND_RET(ret, true);
	WIFI_DEBUG_ARGS("Initialized scan button, ret : %d", ret);

	//Initialize STATION, SOFT_AP and LOS LED
	ret = _InitWifiStatusLEDs();
	WIFI_ASSERT_AND_RET(ret, true);
	WIFI_DEBUG_ARGS("Initialized LOS LED, ret : %d", ret);
	//wifi_status_led_install (GPIO_ID_PIN(14), gpio_mux[10], gpio_func[10]);

	//Register user task to init function for changing wifi mode and for ping
	ret = system_os_task(_WifiUserTasks, USER_TASK_PRIO_2,taskQueue, 3);
	WIFI_ASSERT_AND_RET(ret, true);
	WIFI_DEBUG_ARGS("registered user task for changing OP mode and ping, ret : %d", ret);

	//Register wifi event handler
	wifi_set_event_handler_cb(_wifiEventHandler);
	WIFI_DEBUG("registered wifi event handler");

	//set wifi to station mode (it will be saved in flash)
	ret = wifi_set_opmode(STATION_MODE);
	WIFI_ASSERT_AND_RET(ret, true);
	WIFI_DEBUG_ARGS("set opmode to Station, ret : %d", ret);

	//check flash memory for any saved station config
	struct station_config station_config;
	ret = wifi_station_get_config_default(&station_config);
	WIFI_ASSERT_AND_RET(ret, true);
	WIFI_DEBUG_ARGS("get station flash config, ret : %d", ret);

	//if no station ssid info is saved in flash
	if(station_config.ssid[0] == 0x00){
		// Light up LOS LED
		_SetLOS(1);
		WIFI_DEBUG("LOS LED ON");
	}
	return ret;
}

/*******************************************************************************************
 * FunctionName	:  ConnectToStation
 * Description	:  Connects to Wifi Router(AP)
 * Parameters	:  iData -- Raw HTTP Data received consisting of ssid and password
 * 				   iDataLength -- iData length
 * Return		:  bool, true if successful,
 * 						 false if failed
 ******************************************************************************************/
bool ICACHE_FLASH_ATTR ConnectToStation(char *iData, uint16 iDataLength){
	WIFI_DEBUG("Inside Connect To Station method.");

	bool ret = false;
	if(iData != NULL && iDataLength > 0){
		char *ssid = os_strstr(iData, "S=");
		char *password = os_strstr(iData, "&P=");
		if(ssid != NULL && password != NULL){
			ssid += 2;
			uint16 ssidLength = password - ssid;

			//replace + with space (comes like this from form data)
			char *space = ssid;
			do{
				space = os_strstr(space, "+");
				if(space != NULL && space < space + ssidLength) space = os_memset(space, 32, 1);
			}
			while(space != NULL && space < space + ssidLength );

			password += 3;
			uint16 passwordLength = iDataLength - (ssidLength + 2 + 3);

			os_memset(&AP, 0, sizeof(AP_Info));
			for(uint8 i = 0; i < SCAN_LIST; ++i){
				if(os_strncmp(scanned_APs[i].ssid, ssid, ssidLength) == 0){
					os_memcpy(AP.ssid, scanned_APs[i].ssid, scanned_APs[i].ssid_len);
					os_memcpy(AP.password, password, passwordLength);
					AP.ssid_len = scanned_APs[i].ssid_len;
					break;
				}
			}

			//set wifi in Station mode
			ret = system_os_post(USER_TASK_PRIO_2, STATION_MODE, 0);
			WIFI_DEBUG_ARGS("Station Mode system_os_post, ret : %d", ret);
		}
	}
	return ret;
}

/***************************************************************************************
 * FunctionName	:  ConnectedToInternet
 * Description	:  Checks whether Esp is connected to internet or not
 * Returns		:  bool, true if connected
 * 						 false if not connected
 **************************************************************************************/
bool ICACHE_FLASH_ATTR ConnectedToInternet(void){
	bool ret = false;

	uint8 status = wifi_station_get_connect_status();
	WIFI_DEBUG_ARGS("station status : %d and LOS Status : %d", status, LOS);
	if(status == STATION_GOT_IP && !LOS){
		ret = true;
	}

	return ret;
}
