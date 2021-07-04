/*
 * http.h
 *
 *  Created on: 23-May-2021
 *      Author: harsh
 */

#ifndef INCLUDE_DRIVER_HTTP_H_
#define INCLUDE_DRIVER_HTTP_H_

#include "c_types.h"

//forward declaration
struct espconn;

//un-comment this for debugging messages
//#define HTTP_DEBUG

typedef enum httpStatusCode {
	HTTP_OK, //200,
	HTTP_Bad_Request, //400,
	HTTP_Not_Found  //404
}HTTP_STATUS_CODE;

typedef enum connectionState{
	Closed,
	Keep_Alive
}CONNECTION;

typedef enum contentType{
	text_html,
	text_css,
	application_javascript
}CONTENT_TYPE;

typedef struct httpResponse{
	HTTP_STATUS_CODE httpStatusCode;
	CONNECTION connection;
	char *content;
	uint16 contentLength;
	CONTENT_TYPE contentType;
} HTTP_RESPONSE_PACKET;

typedef enum httpMethod {
	HTTP_GET,
	HTTP_POST,
	HTTP_INVALID
}HTTP_METHOD ;

typedef struct httpRequest{
	HTTP_METHOD httpMethod;
	char *routePath;
	uint16 routeLength;
	char *data;
	uint16 dataLength;
} HTTP_REQUEST_PACKET;

typedef enum httpMessageType{
	HTTP_REQUEST,
	HTTP_RESPONSE
} HTTP_MESSAGE_TYPE;

/***********************************************************************************
 * FunctionName : isHttp
 * Description  : check if received data is of HTTP Protocol and its message type
 * Parameters   : iRecv	   -- received data
 *                iLength  -- length of received data
 *                oMsgType -- Http Message Type
 * Returns      : bool	-- true if data is of HTTP Protocol
 * 						-- false if data is not of HTTP Protocol
***********************************************************************************/
bool isHttp(char *iRecv, uint16 iLength, HTTP_MESSAGE_TYPE *oMsgType);

/***********************************************************************************
 * FunctionName : processHttpRequest
 * Description  : process raw received HTTP Request Data from server
 * Parameters   : iRecv	    	-- received data
 *                iLength  		-- length of received data
 *                oHttpRequest  -- Processed Http Request Obj
 * Returns      : bool	-- true if Successful
 * 						-- false if Failed
***********************************************************************************/
bool processHttpRequest (char *iRecv, uint16 iLength, HTTP_REQUEST_PACKET *oHttpRequest);

/***********************************************************************************
 * FunctionName : sendHttpResponse
 * Description  : Send Http response to server.
 * Parameters   : espconn	    -- esp connection obj
 *                iHttpResponse -- HTTP reponse obj
 * Returns      : bool	-- true if Successful
 * 						-- false if Failed
***********************************************************************************/
bool sendHttpResponse (struct espconn *espconn, HTTP_RESPONSE_PACKET* iHttpResponse);

#endif /* INCLUDE_DRIVER_HTTP_H_ */
