/*
 * wifi_app.c
 *
 *  Created on: May 23, 2025
 *      Author: ASUS
 */

#include "esp_event.h"
#include "esp_event_base.h"
#include "esp_interface.h"
#include "esp_log_level.h"
#include "esp_netif.h"
#include "esp_netif_types.h"
#include "esp_wifi_default.h"
#include "esp_wifi_types_generic.h"
#include "freertos/event_groups.h"
#include "freertos/idf_additions.h"
#include "freertos/task.h"

#include "esp_err.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "http_server.h"


#include "lwip/sockets.h"
#include "portmacro.h"
#include "rgb_led.h"
#include "tasks_common.h"
#include <stdint.h>
#include <stdio.h>
#include <sys/_intsup.h>
#include <time.h>
#include "wifi_app.h"

//tag used for console log messages

static const char TAG[]="wifi_app";

//returns wifi configuration
wifi_config_t *wifi_config = NULL;

//to track the number of tries before the connection attempt fails

static int g_retry_number;


//queue handle to manipulate main queue of events
static QueueHandle_t wifi_app_queue_handle;

//netif objects for station and access points
esp_netif_t* esp_netif_sta = NULL;
esp_netif_t* esp_netif_ap = NULL;

/*
wifi event handler initialization
@param arg data aside from the event data, that is passed to the handler when it is called
@param event base is the base id of the event to the register the handler for
@param event id is the id of the event to register the handler for
@param event data is the event data
*/
static void wifi_app_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data){
	if(event_base ==WIFI_EVENT)
	{
		switch(event_id){
			case WIFI_EVENT_AP_START:
				ESP_LOGI(TAG, "WIFI_EVENT_AP_START");
				break;
			case WIFI_EVENT_AP_STOP:
				ESP_LOGI(TAG, "WIFI_EVENT_AP_STOP");
				break;
			case WIFI_EVENT_AP_STACONNECTED:
				ESP_LOGI(TAG, "WIFI_EVENT_AP_STACONNECTED");
				break;
			case WIFI_EVENT_AP_STADISCONNECTED:
				ESP_LOGI(TAG, "WIFI_EVENT_AP_STADISCONNECTED");
				break;
			case WIFI_EVENT_STA_START:
				ESP_LOGI(TAG, "WIFI_EVENT_STA_START");
				break;
			case WIFI_EVENT_STA_CONNECTED:
				ESP_LOGI(TAG, "WIFI_EVENT_STA_CONNECTED");
				break;
			
			case WIFI_EVENT_STA_DISCONNECTED:
				ESP_LOGI(TAG, "WIFI_EVENT_STA_DISCONNECTED");
				wifi_event_sta_disconnected_t *wifi_event_sta_disconnected = (wifi_event_sta_disconnected_t*)malloc(sizeof(wifi_event_sta_disconnected_t));
				*wifi_event_sta_disconnected = *((wifi_event_sta_disconnected_t*)event_data);
				printf("wifi_event_sta_disconnected: reason: %d", wifi_event_sta_disconnected->reason);
				
				if(g_retry_number < MAX_CONNECTION_RETRIES){
					esp_wifi_connect();
					g_retry_number++;
				}
				else{
					wifi_app_send_message(WIFI_APP_MSG_STA_DISCONNECTED);
				}
				break;
		}
		
	}
	else if(event_base==IP_EVENT){
			switch (event_id) {
				case IP_EVENT_STA_GOT_IP:
					ESP_LOGI(TAG, "IP_EVENT_STA_GOT_IP");
					wifi_app_send_message(WIFI_APP_MSG_STA_CONNECTED_GOT_IP);
					break;	
			}
	}
}



//initilizes the wifi application event handler for wifi and ip events
static void wifi_app_event_handler_init(void){
	 	//event loop for the event handler
		ESP_ERROR_CHECK(esp_event_loop_create_default());
		
		//event handler for the connection
		esp_event_handler_instance_t instance_wifi_event;
		esp_event_handler_instance_t instance_ip_event;
		
		ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_app_event_handler,NULL,&instance_wifi_event));
		ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, ESP_EVENT_ANY_ID, &wifi_app_event_handler,NULL,&instance_ip_event));

		
	}

/*
initializes tcp stack and default wifi configs	
*/
static void wifi_app_default_wifi_init(void){
	
	//initalizes the tcp stack
	ESP_ERROR_CHECK(esp_netif_init());
	
	//default wifi config - operations must be in this order
	wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&wifi_init_config));
	ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
	
	esp_netif_sta = esp_netif_create_default_wifi_sta();
	esp_netif_ap = esp_netif_create_default_wifi_ap();
	
}

/*
configures wap settings adn static ip for the soft ap
*/
static void wifi_app_soft_ap_config(void){
	//soft ap wap config
	wifi_config_t ap_config = 
	{
		.ap = {
			.ssid = WIFI_AP_SSID,
			.ssid_len = strlen(WIFI_AP_SSID),
			.password = WIFI_AP_PASS,
			.channel = WIFI_AP_CHANNEL,
			.ssid_hidden = WIFI_AP_SSID_HIDDEN,
			.authmode = WIFI_AUTH_WPA2_PSK,
			.max_connection = WIFI_AP_MAX_CONNECTIONS,
			.beacon_interval = WIFI_AP_BEACON_INTERVAL,
		},
	};
	
	//cofig dhcp for the ap
	
	esp_netif_ip_info_t ap_ip_info;
	
	memset(&ap_ip_info, 0x00, sizeof(ap_ip_info));
	
	esp_netif_dhcps_stop(esp_netif_ap);			//must call this first
	
	inet_pton(AF_INET, WIFI_AP_IP, &ap_ip_info.ip);	//assign ap's static ip, gw, and netmask
	inet_pton(AF_INET, WIFI_AP_GATEWAY, &ap_ip_info.gw);
	inet_pton(AF_INET, WIFI_AP_NETMASK, &ap_ip_info.netmask);
	
	ESP_ERROR_CHECK(esp_netif_set_ip_info(esp_netif_ap, &ap_ip_info));	//statically configures the network interface
	
	esp_err_t err = esp_netif_dhcps_stop(esp_netif_ap);					//stops dhcp server
	if (err != ESP_OK && err != ESP_ERR_ESP_NETIF_DHCP_ALREADY_STOPPED) {
	    ESP_LOGE(TAG, "Failed to stop DHCP server: %s", esp_err_to_name(err));
	}
	
	ESP_ERROR_CHECK(esp_netif_dhcps_start(esp_netif_ap));
	
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));		//setting the mode for access mode as well as station mode
	ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &ap_config));		//set out config
	ESP_ERROR_CHECK(esp_wifi_set_bandwidth(ESP_IF_WIFI_AP, WIFI_AP_BANDWIDTH));		//our default bw: 20MHz
	ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_STA_POWER_SAVE));
	
}

/**
connects the esp32 to the external AP using updated config 
*/

static void wifi_app_connect_sta(void){
	ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, wifi_app_get_wifi_config()));
	
	ESP_ERROR_CHECK(esp_wifi_connect());
	
}



/*
main task for wifi app 
pvParameter can be passed to the the task
*/
static void wifi_app_task(void *pvParameters){
	wifi_app_queue_message_t msg;
	
	//initialize event handler
	wifi_app_event_handler_init();
	
	//initialize tcp/ip stack and wifi config
	wifi_app_default_wifi_init();
	
	//sosftAP config,
	wifi_app_soft_ap_config();
	
	//start wifi
	ESP_ERROR_CHECK(esp_wifi_start());
	
	//send first event message
	wifi_app_send_message(WIFI_APP_MSG_START_HTTP_SERVER);

	while(1){
		if(xQueueReceive(wifi_app_queue_handle,&msg, portMAX_DELAY))
		{
			switch(msg.msgID){
				case WIFI_APP_MSG_START_HTTP_SERVER:
					ESP_LOGI(TAG, "WIFI_APP_MSG_START_HTTP_SERVER");
					
					http_server_start();
					rgb_led_http_server_started();
					
					break;
				case WIFI_APP_MSG_CONNECTING_FROM_HTTP_SERVER:
					ESP_LOGI(TAG, "WIFI_APP_MSG_CONNECTING_FROM_HTTP_SERVER");
					
					//attempt to make a connection
					wifi_app_connect_sta();
					
					//set the retries to 0
					g_retry_number = 0;
					
					//let the http server know about the connection attempt
					http_server_monitor_send_message(HTTP_MSG_WIFI_CONNECT_INIT);
					
					break;
				case WIFI_APP_MSG_STA_CONNECTED_GOT_IP:
					ESP_LOGI(TAG, "WIFI_APP_MSG_STA_CONNECTED_GOT_IP");
					
					
					//wifi_connected
					rgb_led_wifi_connected();
					
					http_server_monitor_send_message(HTTP_MSG_WIFI_CONNECT_SUCCESS);
					
					
					break;
					
					case WIFI_APP_MSG_STA_DISCONNECTED:
					ESP_LOGI(TAG, "WIFI_APP_MSG_STA_DISCONNECTED");
					
					http_server_monitor_send_message(HTTP_MSG_WIFI_CONNECT_FAIL);
					break;	
					
					case WIFI_APP_MSG_USER_REQUESTED_STA_DISCONNECT:
					ESP_LOGI(TAG, "WIFI_APP_MSG_USER_REQUESTED_STA_DISCONNECT");
					
					g_retry_number=MAX_CONNECTION_RETRIES;
					ESP_ERROR_CHECK(esp_wifi_disconnect());
					rgb_led_http_server_started();
					break;			
					
				default:
					break;
			}
		}
	}	
}

BaseType_t wifi_app_send_message(wifi_app_message_e msgID){
	wifi_app_queue_message_t msg;
	msg.msgID = msgID;
	return xQueueSend(wifi_app_queue_handle, &msg, portMAX_DELAY);
}

wifi_config_t* wifi_app_get_wifi_config(void){
	
return wifi_config;
}

void wifi_start_app(void){
	ESP_LOGI(TAG, "Starting Wifi Application");
	
	//start wifi started led
	rgb_led_wifi_app_started();
	
	//disable default wifi logging messages
	
	esp_log_level_set("wifi", ESP_LOG_NONE);
	
	//allocate memory for the wifi config
	wifi_config = (wifi_config_t*)malloc(sizeof(wifi_config_t));
	memset(wifi_config, 0x00, sizeof(wifi_config_t));
	
	
	//create message queue
	wifi_app_queue_handle = xQueueCreate(3,sizeof(wifi_app_queue_message_t));
	
	//start the wifi application task
	xTaskCreatePinnedToCore(&wifi_app_task, "wifi_app_task", WIFI_APP_TASK_STACK_SIZE, NULL,WIFI_APP_TASK_PRIORITY, NULL, WIFI_APP_TASK_CORE_ID);


}