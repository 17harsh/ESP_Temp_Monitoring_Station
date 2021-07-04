/*
 * user_espconn.c
 *
 *  Created on: 07-Jun-2021
 *      Author: harsh
 */

#include "user_espconn.h"

//system includes
#include "osapi.h"
#include "user_interface.h"
#include "mem.h"
#include "espconn.h"

//user includes
#include "user_webpage.h"
#include "user_wifi.h"

//driver libs
#include "driver/http.h"

#define ASSERT_N_SKIP(var, condition, label)	if(var != condition) goto label

//Set-Up Debugging Macros
#ifndef ESP_ESPCONN_LOGGER
	#define ESPCONN_DEBUG(message)					do {} while(0)
	#define ESPCONN_DEBUG_ARGS(message, args...)	do {} while(0)
#else
	#define ESPCONN_DEBUG(message)					do {os_printf("[ESPCONN-DEBUG] %s", message); os_printf("\r\n");} while(0)
	#define ESPCONN_DEBUG_ARGS(message, args...)	do {os_printf("[ESPCONN-DEBUG] "); os_printf(message, args); os_printf("\r\n");} while(0)
#endif

//static placeholders
static os_event_t taskQueue[1] = {0};
static struct espconn espconn;
static esp_tcp espTcp;
static ip_addr_t server_ip;

static bool sendData = false;

/******** Function Definitions ********/

/***************************************************************************************
 * FunctionName	:  _UserTasks
 * Description	:  User task function callback for disconnecting TCP connection.
 * Parameters	:  event -- user task event
 **************************************************************************************/
void ICACHE_FLASH_ATTR _UserTasks(os_event_t *event){
	ESPCONN_DEBUG_ARGS("Inside espconn user task, event : %d", event->sig);

	sint8 ret = false;
	switch (event->sig) {
	case 0:
		ret = espconn_delete(&espconn);
		ESPCONN_DEBUG_ARGS("espconn_delete : %d", ret);
		break;
	default:
		break;
	}
}

/***************************************************************************************
 * FunctionName	:  _processHttpData
 * Description	:  process received HTTP data
 * Parameters	:  arg -- espconn obj
 * 				   pdata -- received data
 * 				   len -- received data length
 * 				   iMsgType -- HTTP message type
 **************************************************************************************/
void ICACHE_FLASH_ATTR _processHttpData(void *arg, char *pdata, unsigned short len, HTTP_MESSAGE_TYPE iMsgType){
	bool ret = false;
	struct espconn *pesp_conn = arg;

	if(iMsgType == HTTP_REQUEST){
		ESPCONN_DEBUG("It is HTTP Request");

		//initialize HTTP obj
		HTTP_REQUEST_PACKET httpRequest;
		httpRequest.routePath = NULL;
		httpRequest.routeLength = -1;
		httpRequest.data = NULL;
		httpRequest.dataLength = -1;

		//process http request
		ret = processHttpRequest(pdata, len, &httpRequest);
		ASSERT_N_SKIP(ret, true, SKIP_PROCESS);

		if(httpRequest.httpMethod == HTTP_GET && httpRequest.routeLength > 0){
			ESPCONN_DEBUG("HTTP request type : GET");

			#ifdef ESP_ESPCONN_LOGGER
				//print route
				char *routePath = (char*) os_zalloc(httpRequest.routeLength);
				os_memcpy(routePath, httpRequest.routePath, httpRequest.routeLength);
				ESPCONN_DEBUG_ARGS("HTTP Route path : %s", routePath);
				os_free(routePath);
			#endif

			//send response based on route
			HTTP_RESPONSE_PACKET responsePacket;
			responsePacket.connection = Closed;

			if(os_strncmp(httpRequest.routePath, "/", httpRequest.routeLength) == 0){
				responsePacket.httpStatusCode = HTTP_OK;
				responsePacket.content = GetWifi_AP_HTML();
				responsePacket.contentLength = os_strlen(responsePacket.content);
				responsePacket.contentType = text_html;
			}
			else if(os_strncmp(httpRequest.routePath, "styles.css", httpRequest.routeLength) == 0){
				responsePacket.httpStatusCode = HTTP_OK;
				responsePacket.content = GetWifi_AP_CSS();
				responsePacket.contentLength = os_strlen(responsePacket.content);
				responsePacket.contentType = text_css;
			}
			else if(os_strncmp(httpRequest.routePath, "script.js", httpRequest.routeLength) == 0){
				responsePacket.httpStatusCode = HTTP_OK;
				responsePacket.content = GetWifi_AP_JS();
				responsePacket.contentLength = os_strlen(responsePacket.content);
				responsePacket.contentType = application_javascript;
			}
			else {
				responsePacket.httpStatusCode = HTTP_Not_Found;
				responsePacket.content = "";
				responsePacket.contentLength = 0;
				responsePacket.contentType = text_html;
			}

			ret = sendHttpResponse(pesp_conn, &responsePacket);
			ESPCONN_DEBUG_ARGS("HTTP response send : %d", ret);
		}
		else if(httpRequest.httpMethod == HTTP_POST && httpRequest.routeLength > 0){
			ESPCONN_DEBUG("HTTP request type : POST");

			#ifdef ESP_ESPCONN_LOGGER
				//print route
				char *routePath = (char*) os_zalloc(httpRequest.routeLength);
				os_memcpy(routePath, httpRequest.routePath, httpRequest.routeLength);
				ESPCONN_DEBUG_ARGS("HTTP Route path : %s", routePath);
				os_free(routePath);
			#endif

			//send response based on route
			HTTP_RESPONSE_PACKET responsePacket;
			responsePacket.connection = Closed;
			responsePacket.contentType = text_html;

			if(os_strncmp(httpRequest.routePath, "/", httpRequest.routeLength) == 0){

				//send content to client

				responsePacket.httpStatusCode = HTTP_OK;
				responsePacket.content = "";
				responsePacket.contentLength = 0;

			}
			else {
				responsePacket.httpStatusCode = HTTP_Not_Found;
				responsePacket.content = "";
				responsePacket.contentLength = 0;
			}

			ret = sendHttpResponse(pesp_conn, &responsePacket);
			ESPCONN_DEBUG_ARGS("HTTP response send: %d", ret);

			if(httpRequest.data != NULL && httpRequest.dataLength > 0){
				ConnectToStation(httpRequest.data, httpRequest.dataLength);
			}
		}
	}
	else if(iMsgType == HTTP_RESPONSE){

	}

	SKIP_PROCESS:;
}

/***************************************************************************************
 * FunctionName	:  _ESPConn_recv
 * Description	:  Callback when data is received over TCP
 * Parameters	:  arg -- espconn obj
 * 				   pdata -- received data
 * 				   len -- received data length
 **************************************************************************************/
void ICACHE_FLASH_ATTR _ESPConn_recv (void *arg, char *pdata, unsigned short len){
	ESPCONN_DEBUG("Inside espconn data recieve callback.");

	bool ret = false;
	struct espconn *pesp_conn = arg;
	if(pdata != NULL && len > 0)
		ESPCONN_DEBUG_ARGS("Data Received: %s", pdata);

	//********************* HTTP DATA HANDLING *********************//

	//check if received data is of type HTTP and it's Message type
	HTTP_MESSAGE_TYPE httpMsgType;
	ret = isHttp(pdata, len, &httpMsgType);
	if(ret){
		ESPCONN_DEBUG("It is HTTP data");
		_processHttpData(arg, pdata, len, httpMsgType);
	}
	//**************************************************************//
}

/***************************************************************************************
 * FunctionName	:  _ESPConn_sent
 * Description	:  Callback when data is sent over TCP.
 * Parameters	:  arg -- espconn obj
 **************************************************************************************/
void ICACHE_FLASH_ATTR _ESPConn_sent(void *arg){
	ESPCONN_DEBUG("Inside espconn data sent callback.");
	struct espconn *pesp_conn = arg;
}

/***************************************************************************************
 * FunctionName	:  _TCP_Connect
 * Description	:  Callback when TCP connection is established. Used to Register
 * 				   espconn callbacks
 * Parameters	:  arg -- espconn obj
 **************************************************************************************/
void ICACHE_FLASH_ATTR _TCP_Connect (void *arg){
	ESPCONN_DEBUG("Inside TCP connect callback");

	struct espconn *pesp_conn = arg;

	//register espconn callbacks
	sint8 ret = 0;
	ret = espconn_regist_recvcb(pesp_conn, _ESPConn_recv);
	ESPCONN_DEBUG_ARGS("register espconn data receive callback, ret : %d", ret);

	ret = espconn_regist_sentcb(pesp_conn, _ESPConn_sent);
	ESPCONN_DEBUG_ARGS("register espconn data sent callback, ret : %d", ret);

	if(sendData){
		ret = espconn_send(pesp_conn, "Hello", os_strlen("Hello"));
		ESPCONN_DEBUG_ARGS("espconn_send, ret : %d", ret);
	}
}

/***************************************************************************************
 * FunctionName	:  _TCP_Recon
 * Description	:  Callback when reconnecting TCP connection
 * Parameters	:  arg -- espconn obj
 * 				   err -- disconnect error type
 **************************************************************************************/
void ICACHE_FLASH_ATTR _TCP_Recon(void *arg, sint8 err){
	ESPCONN_DEBUG("Inside TCP re-connect callback");

	struct espconn *pesp_conn = arg;

	ESPCONN_DEBUG_ARGS("server's reconnect error : %d", err);
	ESPCONN_DEBUG_ARGS("server's %d.%d.%d.%d:%d disconnect", pesp_conn->proto.tcp->remote_ip[0],
	        		pesp_conn->proto.tcp->remote_ip[1],pesp_conn->proto.tcp->remote_ip[2],
	        		pesp_conn->proto.tcp->remote_ip[3],pesp_conn->proto.tcp->remote_port);

	_TCP_Connect(arg);
}

/***************************************************************************************
 * FunctionName	:  _TCP_Discon
 * Description	:  Callback when TCP connection is closed/disconnected.
 * Parameters	:  arg -- espconn obj
 **************************************************************************************/
void ICACHE_FLASH_ATTR _TCP_Discon(void *arg){
	ESPCONN_DEBUG("Inside TCP disconnect callback");

    struct espconn *pesp_conn = arg;

    ESPCONN_DEBUG_ARGS("server's %d.%d.%d.%d:%d disconnect", pesp_conn->proto.tcp->remote_ip[0],
        		pesp_conn->proto.tcp->remote_ip[1],pesp_conn->proto.tcp->remote_ip[2],
        		pesp_conn->proto.tcp->remote_ip[3],pesp_conn->proto.tcp->remote_port);

}

/*******************************************************************************************
 * FunctionName	:  InitESPConn
 * Description	:  Initializes espconn for communication
 * Return		:  0 if successful, else failed
 ******************************************************************************************/
sint8 ICACHE_FLASH_ATTR InitESPConn(void){
	ESPCONN_DEBUG("Init ESP Connection");

	os_memset(&server_ip, 0, sizeof(server_ip));

	//initialize espconn structure
	espconn.type = ESPCONN_TCP;
	espconn.state = ESPCONN_NONE;
	espconn.proto.tcp = &espTcp;
	espconn.proto.tcp->local_port = TCP_LOCAL_PORT;

	//Register TCP callbacks
	sint8 ret = false;
	ret = espconn_regist_connectcb(&espconn, _TCP_Connect);
	ESPCONN_DEBUG_ARGS("register TCP listen connect callback, ret : %d", ret);

	ret = espconn_regist_reconcb(&espconn, _TCP_Recon);
	ESPCONN_DEBUG_ARGS("register TCP reconnect callback, ret : %d", ret);

	ret = espconn_regist_disconcb(&espconn, _TCP_Discon);
	ESPCONN_DEBUG_ARGS("register TCP disconnect callback, ret : %d", ret);

	return ret;
}

/*******************************************************************************************
 * FunctionName	:  StopLocalServer
 * Description	:  Stops the Local Server
 * Return		:  0 if successful, else failed
 ******************************************************************************************/
sint8 ICACHE_FLASH_ATTR StopLocalServer(void){
	ESPCONN_DEBUG_ARGS("Stopping Local TCP Server on port: %d", espconn.proto.tcp->local_port);

	sint8 ret = false;
	//get connection info
	remot_info *remote_info = NULL;
	ret = espconn_get_connection_info(&espconn, &remote_info , 0);

	//Disconnect any existing TCP conn
	if(ret == ESPCONN_OK){
		for(int i = 0; i < espconn.link_cnt; i++){
			if(remote_info[i].state == ESPCONN_CONNECT){
				espconn.proto.tcp->remote_ip[0] = remote_info[i].remote_ip[0];
				espconn.proto.tcp->remote_ip[1] = remote_info[i].remote_ip[1];
				espconn.proto.tcp->remote_ip[2] = remote_info[i].remote_ip[2];
				espconn.proto.tcp->remote_ip[3] = remote_info[i].remote_ip[3];
				espconn.proto.tcp->remote_port = remote_info[i].remote_port;

				ret = espconn_disconnect(&espconn);
				ESPCONN_DEBUG_ARGS("esp disconnect : %d", ret);
			}
		}
	}
	//Register user task to delete connection
	ret = system_os_task(_UserTasks, USER_TASK_PRIO_1,taskQueue, 1);
	//call user task to delete connection
	ret = system_os_post(USER_TASK_PRIO_1, 0, 0);
	return ret;
}

/*******************************************************************************************
 * FunctionName	:  StartLocalServer
 * Description	:  Starts the Local Server
 * Return		:  0 if successful, else failed
 ******************************************************************************************/
sint8 ICACHE_FLASH_ATTR StartLocalServer(void){
	sint8 ret = false;
	ret = espconn_accept(&espconn);
	ESPCONN_DEBUG_ARGS("Starting Local TCP server listening on port: %d, ret : %d", espconn.proto.tcp->local_port, ret);
	return ret;
}

/*******************************************************************************************
 * FunctionName	:  _DNS_cb
 * Description	:  get host by name DNS cb
 * Paramaters	:  name -- pointer to the name that was looked up
 * 				   ipaddr -- pointer to an ip_addr_t containing the IP address of the hostname
 * 				   arg -- espconn obj
 ******************************************************************************************/
void ICACHE_FLASH_ATTR _DNS_cb(const char *name, ip_addr_t *ipaddr, void *arg){
	struct espconn *pespconn = (struct espconn *)arg;
	if (ipaddr != NULL){

		server_ip = *ipaddr;

		//read ip
		ESPCONN_DEBUG_ARGS("_DNS_cb name: %s", name);
		ESPCONN_DEBUG_ARGS("_DNS_cb ipaddr: " IPSTR, IP2STR(&ipaddr->addr));

		//create tcp connection with server
		//send http data to server
		//close connection
	}
}

/*******************************************************************************************
 * FunctionName	:  SendDataToRemoteServer
 * Description	:
 * Return		:
 ******************************************************************************************/
sint8 ICACHE_FLASH_ATTR SendDataToRemoteServer(char *iUrl, uint16 iUrlLength, char*iData, uint16 iDataLength){
	ESPCONN_DEBUG("Send data to remote webserver");
	sint8 ret = false;
	if(iUrl != NULL && iUrlLength > 0 && iData != NULL && iDataLength > 0){

		/*struct espconn *pespconn = NULL;
		ret = espconn_gethostbyname(pespconn, iUrl, &server_ip, _DNS_cb);
		ESPCONN_DEBUG_ARGS("espconn_gethostbyname: " IPSTR " ret: %d", IP2STR(&server_ip.addr), ret);

		if(ret == ESPCONN_OK){
			//create tcp connection with server
			//send http data to server
			//close connection
		}*/

		/*** Test Code ***/

		uint32 ip = ipaddr_addr("192.168.0.105");

		espconn.state = ESPCONN_NONE;
		espconn.type = ESPCONN_TCP;
		espconn.proto.tcp = &espTcp;

		os_memcpy(espconn.proto.tcp->remote_ip, &ip, 4);
		espconn.proto.tcp->remote_port = 8080;
		espconn.proto.tcp->local_port = espconn_port();

		ret = espconn_connect(&espconn);
		ESPCONN_DEBUG_ARGS("make tcp connection to server, ret: %d:", ret);

		sendData = true;

		/****************/
	}

	return ret;
}
