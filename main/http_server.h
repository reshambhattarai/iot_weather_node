/*
 * http_server.h
 *
 *		Created on: May 23, 2025
 *     	Author: ASUS
 */

#ifndef MAIN_HTTP_SERVER_H_
#define MAIN_HTTP_SERVER_H_


#define OTA_UPDATE_PENDING		0
#define OTA_UPDATE_SUCCESSFUL	1
#define OTA_UPDATE_FAILED		-1

/*
connection status for wifi
**/
typedef enum http_server_wifi_connect_status{
	NONE = 0,
	HTTP_STATUS_WIFI_CONNECTING,
	HTTP_STATUS_WIFI_CONNECT_FAILED,
	HTTP_STATUS_WIFI_CONNECT_SUCCESS,
} http_server_wifi_connect_status_e;



/**
 * Messages for the HTTP monitor
 */
#include "portmacro.h"
typedef enum http_server_msg{
	HTTP_MSG_WIFI_CONNECT_INIT	=	0,
	HTTP_MSG_WIFI_CONNECT_SUCCESS,
	HTTP_MSG_WIFI_CONNECT_FAIL,
	HTTP_MSG_OTA_UPDATE_SUCCESSFUL,
	HTTP_MSG_OTA_UPDATE_FAILED,
} http_server_message_e;

/**
Structure for the message Queue
*/
typedef struct http_server_queue_message
{
	http_server_message_e msgId;
}http_server_queue_message_t;

/**
sends a message to queue
@param msgId that takes enum for the http_server_message_e
@param pdTrue if the message was successfully send else pdFalse
*/
BaseType_t http_server_monitor_send_message(http_server_message_e msgId);

//starts http_server
void http_server_start(void);


//stops http_server
void http_server_stop(void);


/*
timer callback function which restarts esp at successful firmware update
*/
void http_server_fw_update_reset_callback(void *arg);

 
#endif /* MAIN_HTTP_SERVER_H_ */
