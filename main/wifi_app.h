/*
 * wifi_app.h
 *
 *  Created on: May 23, 2025
 *      Author: ASUS
 */

#ifndef MAIN_WIFI_APP_H_
#define MAIN_WIFI_APP_H_

#include "esp_netif_types.h"
#include "esp_wifi_types_generic.h"
#include "portmacro.h"

//wifi application settings

#define WIFI_AP_SSID			"ESP_32_AP"
#define WIFI_AP_PASS			"password"
#define WIFI_AP_CHANNEL			1
#define WIFI_AP_SSID_HIDDEN		0
#define WIFI_AP_MAX_CONNECTIONS	5
#define WIFI_AP_BEACON_INTERVAL	100
#define WIFI_AP_IP				"192.168.0.1"
#define WIFI_AP_GATEWAY			"192.168.0.1"
#define WIFI_AP_NETMASK			"255.255.255.0"
#define WIFI_AP_BANDWIDTH		WIFI_BW_HT20
#define WIFI_STA_POWER_SAVE		WIFI_PS_NONE
#define MAX_SSID_LENGTH			32
#define MAX_PASS_LENGTH			64
#define MAX_CONNECTION_RETRIES	5

//netif object for the station and access points
extern esp_netif_t* 	esp_netif_sta;
extern esp_netif_t* 	esp_netif_ap;

/*
//message ids for wifi application tasks
*/
typedef enum wifi_app_message{
	WIFI_APP_MSG_START_HTTP_SERVER = 0,
	WIFI_APP_MSG_CONNECTING_FROM_HTTP_SERVER,
	WIFI_APP_MSG_STA_CONNECTED_GOT_IP,
	WIFI_APP_MSG_USER_REQUESTED_STA_DISCONNECT,
	WIFI_APP_MSG_STA_DISCONNECTED
	
} wifi_app_message_e;

/*
structure for enum messages
*/
typedef struct wifi_app_queue_message{
	wifi_app_message_e	msgID;
} wifi_app_queue_message_t;
/*
sends msgIDs from the wifi_message_e and returns pdTRUE if successfully msg sent else sends pdFalse
*/

BaseType_t wifi_app_send_message(wifi_app_message_e msgID);

/*
starts the wifi rtos task
*/
void wifi_start_app(void);

//gets wifi config
wifi_config_t* wifi_app_get_wifi_config(void);


#endif /* MAIN_WIFI_APP_H_ */
