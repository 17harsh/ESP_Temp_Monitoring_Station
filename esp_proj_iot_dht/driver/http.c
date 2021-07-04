/*
 * http.c
 *
 *  Created on: 23-May-2021
 *      Author: harsh
 */

#include "../../esp_proj_wifi/include/driver/http.h"

#include "stdlib.h"

#include "osapi.h"
#include "user_interface.h"
#include "mem.h"
#include "espconn.h"

//Set-Up Debugging Macros
#ifndef HTTP_DEBUG
	#define HTTP_LOG_DEBUG(message)					do {} while(0)
	#define HTTP_LOG_DEBUG_ARGS(message, args...)	do {} while(0)
#else
	#define HTTP_LOG_DEBUG(message)					do {os_printf("[HTTP-DEBUG] %s", message); os_printf("\r\n");} while(0)
	#define HTTP_LOG_DEBUG_ARGS(message, args...)	do {os_printf("[HTTP-DEBUG] "); os_printf(message, args); os_printf("\r\n");} while(0)
#endif

/***********************************************************************************
 * FunctionName : _httpRoutePath
 * Description  : Extract HTTP route path from raw data.
 * Parameters   : iRecv		   -- raw received data
 *                iLength	   -- length of received data
 *                oRoutePath   -- route path
 *                oPathLength  -- route path length
 * Returns      : bool	-- true if Successful
 * 						-- false if Failed
***********************************************************************************/
bool _httpRoutePath(char *iRecv, uint16 iLength, char **oRoutePath, uint16 *oPathLength){
	HTTP_LOG_DEBUG("Inside httpRoutePath");
	bool result = false;
	if(iRecv != NULL && iLength > 0){
		char* p1 = NULL;
		char* p2 = NULL;
		p1 = os_strstr(iRecv, "/");
		p2 = os_strstr(iRecv, "HTTP");
		if(p1 != NULL && p2 != NULL){
			++p1;--p2;
			*oRoutePath = p1;
			*oPathLength = p2 - p1;
			result = true;
		}
	}
	return result;
}

/***********************************************************************************
 * FunctionName : _httpRequestData
 * Description  : Extract HTTP request data from raw data.
 * Parameters   : iRecv		   -- raw received data
 *                iLength	   -- length of received data
 *                oData		   -- extracted HTTP data
 *                oDataLength  -- HTTP data length
 * Returns      : bool	-- true if Successful
 * 						-- false if Failed
***********************************************************************************/
bool _httpRequestData(char *iRecv, uint16 iLength, char **oData, uint16 *oDataLength){
	HTTP_LOG_DEBUG("Inside httpRequestData");
	bool result = false;
	if(iRecv != NULL && iLength > 0){
		char* p1 = NULL;
		char* p2 = NULL;
		p1 = os_strstr(iRecv, "\r\n\r\n");
		p2 = os_strstr(iRecv, "Content-Length:");
		if(p1 != NULL && p2 != NULL){
			p1 += 4;
			*oData = p1;

			p2 += 16;
			char *p3 = os_strstr(p2, "\r\n");
			char p4[5] = {0};
			os_memcpy(p4, p2, p3-p2);
			uint16 datalength = atoi(p4);
			*oDataLength = datalength;
			result = true;
		}
	}
	return result;
}

/***********************************************************************************
 * FunctionName : sendHttpResponse
 * Description  : Send Http response to server.
 * Parameters   : espconn	    -- esp connection obj
 *                iHttpResponse -- HTTP reponse obj
 * Returns      : bool	-- true if Successful
 * 						-- false if Failed
***********************************************************************************/
bool sendHttpResponse (struct espconn *espconn, HTTP_RESPONSE_PACKET* iHttpResponse){
	HTTP_LOG_DEBUG("inside sendHttpResponse");
	bool result = false;
	if(iHttpResponse != NULL){
		HTTP_LOG_DEBUG_ARGS("content : %s", iHttpResponse->content);

		char* responsePacket = (char*) os_zalloc(256 + iHttpResponse->contentLength);

		char *httpStatusCode = NULL;
		if(iHttpResponse->httpStatusCode == HTTP_OK) httpStatusCode = "200 OK";
		else if(iHttpResponse->httpStatusCode == HTTP_Bad_Request) httpStatusCode = "400 Bad Request";
		else if(iHttpResponse->httpStatusCode == HTTP_Not_Found) httpStatusCode = "404 Not Found";

		char *contentType = NULL;
		if(iHttpResponse->contentType == text_html) contentType = "text/html";
		else if(iHttpResponse->contentType == text_css) contentType = "text/css";
		else if(iHttpResponse->contentType == application_javascript) contentType = "application/javascript";

		char *connection = NULL;
		if(iHttpResponse->connection == Closed) connection = "Closed";

		if(responsePacket != NULL && httpStatusCode != NULL && contentType != NULL && connection !=NULL && iHttpResponse->content != NULL){
			os_sprintf(responsePacket, "HTTP/1.1 %s\r\n\
Server: ESP8266\r\n\
Content-Length: %d\r\n\
Content-Type: %s\r\n\
Connection: %s\r\n\r\n\
%s", httpStatusCode, iHttpResponse->contentLength, contentType, connection, iHttpResponse->content);

			HTTP_LOG_DEBUG_ARGS("Content : %s",responsePacket);
			if(espconn != NULL){
				sint8 status = espconn_send(espconn, responsePacket, os_strlen(responsePacket));
				HTTP_LOG_DEBUG_ARGS("espconn send, status : %d",status);
				if(status == 0) result = true;
			}
		}
		os_free(responsePacket);
	}
	return result;
}

/***********************************************************************************
 * FunctionName : processHttpRequest
 * Description  : process raw received HTTP Request Data
 * Parameters   : iRecv	    	-- received data
 *                iLength  		-- length of received data
 *                oHttpRequest  -- Processed Http Request Obj
 * Returns      : bool	-- true if Successful
 * 						-- false if Failed
***********************************************************************************/
bool processHttpRequest (char *iRecv, uint16 iLength, HTTP_REQUEST_PACKET *oHttpRequest){
	bool result = false;
	if(iRecv != NULL && iLength > 0 && oHttpRequest != NULL){

		HTTP_METHOD httpRequest = HTTP_INVALID;
		if(os_strncmp(iRecv, "GET", 3) == 0){
			httpRequest =  HTTP_GET;
			HTTP_LOG_DEBUG("Request type : GET");
		}
		else if(os_strncmp(iRecv, "POST", 4) == 0){
			httpRequest =  HTTP_POST;
			HTTP_LOG_DEBUG("Request type : POST");
		}

		oHttpRequest->httpMethod = httpRequest;

		if(httpRequest != HTTP_INVALID){
			char *routePath = NULL;
			uint16 pathLength = -1;
			if(_httpRoutePath(iRecv, iLength, &routePath, &pathLength) == true){
				if(pathLength == 0 && routePath != NULL){
					//that is, route path is root
					++pathLength;
					--routePath;
				}
				oHttpRequest->routeLength = pathLength;
				oHttpRequest->routePath = routePath;
				result = true;
			}

			char *data = NULL;
			uint16 dataLength = -1;
			if(_httpRequestData(iRecv, iLength, &data, &dataLength) == true){
				oHttpRequest->dataLength = dataLength;
				oHttpRequest->data = data;
				result = true;
			}
		}
	}
	return result;
}

/***********************************************************************************
 * FunctionName : isHttp
 * Description  : check if received data is of HTTP Protocol and its message type
 * Parameters   : iRecv	   -- received data
 *                iLength  -- length of received data
 *                oMsgType -- Http Message Type
 * Returns      : bool	-- true if data is of HTTP Protocol
 * 						-- false if data is not of HTTP Protocol
***********************************************************************************/
bool isHttp(char *iRecv, uint16 iLength, HTTP_MESSAGE_TYPE *oMsgType){
	bool result = false;
	if(iLength > 0 && iRecv != NULL){
		char *isHttp = NULL;
		isHttp = os_strstr(iRecv, "HTTP");
		if(isHttp != NULL){
			result = true;

			if(os_strncmp(iRecv, "HTTP", 4) == 0)
				*oMsgType = HTTP_RESPONSE;
			else
				*oMsgType = HTTP_REQUEST;
		}
	}
	return result;
}
