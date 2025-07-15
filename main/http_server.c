/*
 * http_server.c
 *
 *		Created on: May 23, 2025
 *     	Author: ASUS
 */
 
#include "esp_err.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_netif_ip_addr.h"
#include "esp_netif_types.h"
#include "esp_ota_ops.h" 
#include "esp_partition.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "esp_wifi_types_generic.h"
#include "sys/param.h"
#include "esp_wifi.h"
 
#include "http_server.h"
#include "freertos/idf_additions.h"
#include "http_parser.h"
#include "portmacro.h"
#include "tasks_common.h"
#include "DHT22.h"
#include "wifi_app.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
 
 //tag used for esp_console messages
 
 static const char TAG[] = "http_server";
 
 //firmware update status 
 static int g_fw_update_status = OTA_UPDATE_PENDING;
 
 //wifi_connect_status
 static int g_wifi_connect_status =NONE;
 
 //http monitor task handle
 static TaskHandle_t task_http_server_monitor = NULL;
 
 //queue handle to manipulate the main queue of events
 static QueueHandle_t	http_server_monitor_queue_handle;
 
 
 /*
 esp32 timer configuration passed to esp timer create
 */
 const esp_timer_create_args_t fw_update_reset_args = {
	.callback = &http_server_fw_update_reset_callback,
	.arg = NULL,
	.dispatch_method = ESP_TIMER_TASK,
	.name = "fw_update_reset"
 };
 esp_timer_handle_t fw_update_reset;
 
 
 
 
 
 //http task handle
 static httpd_handle_t	http_server_handle = NULL;
 
 //embedded file : app.css	app.js	favicon.ico	index.html	jquery files
 
 extern const uint8_t jquery_3_3_1_min_js_start[]	asm("_binary_jquery_3_3_1_min_js_start");
 extern const uint8_t jquery_3_3_1_min_js_end[]		asm("_binary_jquery_3_3_1_min_js_end");
 extern const uint8_t index_html_start[]			asm("_binary_index_html_start");
 extern const uint8_t index_html_end[]				asm("_binary_index_html_end");
 extern const uint8_t app_css_start[]				asm("_binary_app_css_start");
 extern const uint8_t app_css_end[]					asm("_binary_app_css_end");
 extern const uint8_t app_js_start[]				asm("_binary_app_js_start");
 extern const uint8_t app_js_end[]					asm("_binary_app_js_end");
 extern const uint8_t favicon_ico_start[]			asm("_binary_favicon_ico_start");
 extern const uint8_t favicon_ico_end[]				asm("_binary_favicon_ico_end");
 
 
 /*
 jquery get handler is requested when accessing the webpage
 @param req HTTP request for which the uri needs to be handled
@returns ESP_OK
  */
  
  /*
   checks for g_fw_update_status and creates fw_update_reset_timer if the status is true
   */
   static void http_server_fw_update_reset_timer(void){
	if(g_fw_update_status==OTA_UPDATE_SUCCESSFUL){
  		ESP_LOGI(TAG, "http_server_fw_update_reset_timer: FIRMWARE_UPDATE_SUCCESSFUL, starting fw update reset timer");
		
		//give webpage time to initialize and acknowlegdge timer
		ESP_ERROR_CHECK(esp_timer_create(&fw_update_reset_args, &fw_update_reset));
		ESP_ERROR_CHECK(esp_timer_start_once( fw_update_reset, 8000000));
  	}
	else{
		ESP_LOGI(TAG, "http_server_fw_update_reset_timer: FIRMWARE_UPDATE_UNSUCCESSFUL");
	}
   }
   
   
   
  
  //http server monitor tasks used to track events of http server
  //@param pvParameter which can be passed to the task  
static void http_server_monitor(void *parameter){
	http_server_queue_message_t msg;
	
	for(;;)
	{
		if(xQueueReceive(http_server_monitor_queue_handle,&msg,portMAX_DELAY)){
			switch(msg.msgId){
				case HTTP_MSG_WIFI_CONNECT_INIT:
					ESP_LOGI(TAG, "HTTP_MSG_WIFI_CONNECT_INIT");
					g_wifi_connect_status = HTTP_STATUS_WIFI_CONNECTING;
					break;
				case HTTP_MSG_WIFI_CONNECT_SUCCESS:
					ESP_LOGI(TAG, "HTTP_MSG_WIFI_CONNECT_SUCCESS");
					g_wifi_connect_status = HTTP_STATUS_WIFI_CONNECT_SUCCESS;
					break;
				case HTTP_MSG_WIFI_CONNECT_FAIL:
					ESP_LOGI(TAG, "HTTP_MSG_WIFI_CONNECT_FAIL");
					g_wifi_connect_status = HTTP_STATUS_WIFI_CONNECT_FAILED;
					break;
				case HTTP_MSG_OTA_UPDATE_SUCCESSFUL:
					ESP_LOGI(TAG, "HTTP_MSG_OTA_UPDATE_SUCCESSFUL");
					g_fw_update_status = OTA_UPDATE_SUCCESSFUL;
					http_server_fw_update_reset_timer();
					break;
				case HTTP_MSG_OTA_UPDATE_FAILED:
					ESP_LOGI(TAG, "HTTP_MSG_OTA_UPDATE_FAILED");
					g_fw_update_status = OTA_UPDATE_FAILED;
				break;
				}
		}
	}
	
}  

  
static esp_err_t http_server_jquery_handler(httpd_req_t *req)
{
	ESP_LOGI(TAG,"Jquery Requested");
	
	httpd_resp_set_type(req, "application/javascript");
	httpd_resp_send(req, (const char*)jquery_3_3_1_min_js_start, jquery_3_3_1_min_js_end-jquery_3_3_1_min_js_start);
	
	return ESP_OK;
}
 
/*
 sends the index.html page
 @param req HTTP request for which the uri needs to be handled
@returns ESP_OK
  */
static esp_err_t http_server_index_html_handler(httpd_req_t *req)
{
	ESP_LOGI(TAG,"index.html Requested");
	
	httpd_resp_set_type(req, "text/html");
	httpd_resp_send(req, (const char*)index_html_start, index_html_end-index_html_start);
	
	return ESP_OK;
}


/*
 sends the app.css page
 @param req HTTP request for which the uri needs to be handled
@returns ESP_OK
  */
static esp_err_t http_server_app_css_handler(httpd_req_t *req)
{
	ESP_LOGI(TAG,"app.css Requested");
	
	httpd_resp_set_type(req, "text/css");
	httpd_resp_send(req, (const char*)app_css_start, app_css_end-app_css_start);
	
	return ESP_OK;
}


/*
 sends the app.js page
 @param req HTTP request for which the uri needs to be handled
@returns ESP_OK
  */
static esp_err_t http_server_app_js_handler(httpd_req_t *req)
{
	ESP_LOGI(TAG,"app.js Requested");
	
	httpd_resp_set_type(req, "application/javascript");
	httpd_resp_send(req, (const char*)app_js_start, app_js_end-app_js_start);
	
	return ESP_OK;
}


/*
 sends the favicon.ico page
 @param req HTTP request for which the uri needs to be handled
@returns ESP_OK
  */
static esp_err_t http_server_favicon_ico_handler(httpd_req_t *req)
{
	ESP_LOGI(TAG,"favicon.ico Requested");
	
	httpd_resp_set_type(req, "image/x-icon");
	httpd_resp_send(req, (const char*)favicon_ico_start, favicon_ico_end-favicon_ico_start);
	
	return ESP_OK;
}


/*
 receives the bin file from the web page and handles the update
 @param req HTTP request for which the uri needs to be handled
@returns ESP_OK else ESP_FAIL if time out occurs and the update can be started
  */
static esp_err_t http_server_OTA_update_handler(httpd_req_t *req){
	esp_ota_handle_t ota_handle;
	
	char OTA_buffer[1024];
	int content_length = req-> content_len;
	int content_received = 0;
	int recv_len;
	
	bool is_req_body_started = false;
	bool flash_successful = false;
	
	const esp_partition_t *update_partition = esp_ota_get_next_update_partition(NULL);
	
	
	do{
		if((recv_len =httpd_req_recv(req,OTA_buffer, MIN(content_length,sizeof(OTA_buffer)))) <0){
		
			//check if timeout occured
			if(recv_len == HTTPD_SOCK_ERR_TIMEOUT){
				ESP_LOGI(TAG, "http_server_ota_update_handler : socket timeout");		
				continue;  //retry receiving if timeout occurs
			}
				ESP_LOGI(TAG, "http_server_ota_update_handler : OTA other errors %d", recv_len);	
				return ESP_FAIL;
		}
		printf("http_server_ota_update_handler: OTA_RX: %d of %d\r", content_received, content_length);
		
		//checking to see if this is the first message we received and looking for the header info
		if(!is_req_body_started){
			is_req_body_started = true;
			
			//get location of the .bin file content and remove web form data
			char *body_start_p = strstr(OTA_buffer, "\r\n\r\n")  + 4;
			int body_part_len = recv_len  - (body_start_p - OTA_buffer);
		printf("http_server_ota_update_handler: %d \r\n", content_length);
		
		esp_err_t err = esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &ota_handle);
		
		if(err!=ESP_OK){
			printf("http_server_ota_update_handler: Error with OTA begin, cancelling OTA \r\n");
			return ESP_FAIL;
		}
		else{
			printf("http_server_ota_update_handler: Writing to partition-subtype: %d at offset 0x%lu \r\n", update_partition->subtype, (unsigned long)update_partition->address);
		}
		
		//write this first part of data
		esp_ota_write(ota_handle, body_start_p, body_part_len);
		}
		else{
			//write ota  data
			esp_ota_write(ota_handle, OTA_buffer, recv_len);
			content_received += recv_len;
		}
	} while(recv_len>0 && content_received< content_length);		
	if(esp_ota_end(ota_handle)==ESP_OK){
		//lets update the partion
		if(esp_ota_set_boot_partition(update_partition)==ESP_OK){
			const esp_partition_t *boot_partition = esp_ota_get_boot_partition();
			ESP_LOGI(TAG, "http_server_ota_update_handler: Next boot partition-subtype %d is at offset 0x%lu", boot_partition->subtype, (unsigned long)boot_partition->address);
			flash_successful = true;
		}
		else{
			ESP_LOGI(TAG, "http_server_ota_update_handler: FLASH Error!!!");
		}
	}
	else{
				ESP_LOGI(TAG, "http_server_ota_update_handler: esp_ota_end_error!!!");
			}
	if(flash_successful){
		http_server_monitor_send_message(HTTP_MSG_OTA_UPDATE_SUCCESSFUL);
	}
	else{
		http_server_monitor_send_message(HTTP_MSG_OTA_UPDATE_FAILED);
	}
	
		return ESP_OK;
}
 
/**
ota status handler responds with the firmware update status after the ota update is started
and responds with teh compile time/date when the page is first requested
@param req HTTP request for which the uri needs to be handled
@returns ESP_OK 
*/

static esp_err_t http_server_OTA_status_handler(httpd_req_t *req){
	char otaJSON[100];
	
	ESP_LOGI(TAG, "OTA status requested");
	
	snprintf(otaJSON, sizeof(otaJSON), "{\"ota_update_status\":%d,\"compile_time\":\"%s\", \"compile_date\":\"%s\"}", g_fw_update_status, __TIME__, __DATE__);
	httpd_resp_set_type(req, "application/json");
	httpd_resp_send(req, otaJSON, strlen(otaJSON));
	return ESP_OK;
}


/**
dht sensor reading json handler responds with dht22 sensor data
@param req HTTP request for which the uri needs to be handled
@returns ESP_OK 
*/

static esp_err_t http_server_dht_sensor_json_handler(httpd_req_t *req){
	
	ESP_LOGI(TAG, "/dhtSensor.json requested");
	
	char dhtSensorJSON[100];
	
	sprintf(dhtSensorJSON, "{\"temp\":\"%.1f\",\"humidity\":\"%.1f\"}",getTemperature(),getHumidity());
	
	httpd_resp_set_type(req, "application/json");
	httpd_resp_send(req, dhtSensorJSON, strlen(dhtSensorJSON));
	
	return ESP_OK;
}

/**
wifi_connect_json handler is invoked after the connect button is pressed and handles receiving ssid and password entered by the user
@param req HTTP request for which the uri needs to be handled
@returns ESP_OK 
*/
static esp_err_t http_server_wifi_connect_json_handler(httpd_req_t *req){
	ESP_LOGI(TAG, "/wifi connect json requested");
	
	size_t len_ssid = 0 , len_pass = 0;
	char *ssid_str = NULL, *pass_str = NULL;
	
	//get ssid header
	len_ssid = httpd_req_get_hdr_value_len(req, "my-connect-ssid")+1;
	
	if(len_ssid>1){
		ssid_str = malloc(len_ssid);
		if(httpd_req_get_hdr_value_str(req, "my-connect-ssid",ssid_str, len_ssid)==ESP_OK){
			ESP_LOGI(TAG, "http_server_wifi_connect_json_handler: found header --> my-connect-ssid:  %s", (char *)ssid_str);
		}
	}
	
	//get pass header
	len_pass = httpd_req_get_hdr_value_len(req, "my-connect-pwd")+1;
	
	if(len_pass>1){
			pass_str = malloc(len_pass);
			if(httpd_req_get_hdr_value_str(req, "my-connect-pwd",pass_str, len_pass)==ESP_OK){
				ESP_LOGI(TAG, "http_server_wifi_connect_json_handler: found header --> my-connect-pass:  %s", (char *)pass_str);
			}
		}
		
	//update the wifi configuration and lett wifi_app know
	wifi_config_t* wifi_config = wifi_app_get_wifi_config();
	memset(wifi_config, 0x00, sizeof(wifi_config_t));
	memcpy(wifi_config->sta.ssid, ssid_str, len_ssid);
	memcpy(wifi_config->sta.password, pass_str, len_pass);
	
	wifi_app_send_message(WIFI_APP_MSG_CONNECTING_FROM_HTTP_SERVER);
	free(ssid_str);
	free(pass_str);
		
	return ESP_OK;
}


//wifi connect status handler updates the connection status for the web page
//@param req HTTP request for which the uri needs to be handled
//@returns ESP_OK 
static esp_err_t http_server_wifi_connect_status_json_handler(httpd_req_t *req){
	ESP_LOGI(TAG,"http_server_wifi_connect_status_json_handler: requested");	
	
	char statusJSON[100];
	sprintf(statusJSON, "{\"wifi_connect_status\":%d}", g_wifi_connect_status);
	
	httpd_resp_set_type(req, "application/json");
	httpd_resp_send(req, statusJSON, strlen(statusJSON));
	
	
	return ESP_OK;
}

//wifi connect info json handler updates the connection status for the web page
//@param req HTTP request for which the uri needs to be handled
//@returns ESP_OK 
static esp_err_t http_server_wifi_get_connect_info_json_handler(httpd_req_t *req){
	ESP_LOGI(TAG,"/wifiConnectInfo.json: requested");	
	
	char ipInfoJSON[200];
	memset(ipInfoJSON, 0,sizeof(ipInfoJSON));
	
	char ip[IP4ADDR_STRLEN_MAX];
	char netmask[IP4ADDR_STRLEN_MAX];
	char gw[IP4ADDR_STRLEN_MAX];
	
	
		wifi_ap_record_t wifi_data;
		ESP_ERROR_CHECK(esp_wifi_sta_get_ap_info(&wifi_data));
		char *ssid = (char *)wifi_data.ssid;
		
		esp_netif_ip_info_t ip_info;
		ESP_ERROR_CHECK(esp_netif_get_ip_info(esp_netif_sta,&ip_info));
		esp_ip4addr_ntoa(&ip_info.ip, ip, IP4ADDR_STRLEN_MAX);
		esp_ip4addr_ntoa(&ip_info.netmask, netmask, IP4ADDR_STRLEN_MAX);
		esp_ip4addr_ntoa(&ip_info.gw, gw, IP4ADDR_STRLEN_MAX);
		
		sprintf(ipInfoJSON, "{\"ip\":\"%s\", \"netmask\":\"%s\", \"gw\":\"%s\", \"ap\":\"%s\"}", ip, netmask, gw, ssid);
		
		
		ESP_LOGI(TAG, "Sending JSON: %s", ipInfoJSON);

		
		
		httpd_resp_set_type(req, "application/json");
		httpd_resp_send(req, ipInfoJSON, strlen(ipInfoJSON));
	
	
	
	
	
	return ESP_OK;
}




//wifi disconnect json handler updates the connection status for the web page
//@param req HTTP request for which the uri needs to be handled
//@returns ESP_OK 
static esp_err_t http_server_wifi_disconnect_json_handler(httpd_req_t *req){

	
	ESP_LOGI(TAG,"/wifidiconnect.json: requested");	
	
	
	return ESP_OK;	
}

 
 /**
 sets up the httpd server configuration
 * @returns http_server_handle if successfull else return NULL
 */
 static httpd_handle_t http_server_configure(void){
	//generate the default server config
	httpd_config_t	config = HTTPD_DEFAULT_CONFIG();
	
	//http server monitor task
	xTaskCreatePinnedToCore(&http_server_monitor, "http_server_monitor", HTTP_SERVER_MONITOR_STACK_SIZE, NULL, HTTP_SERVER_MONITOR_PRIORITY, &task_http_server_monitor, HTTP_SERVER_MONITOR_CORE_ID);
	
	//create message queue
	http_server_monitor_queue_handle = xQueueCreate(3, sizeof(http_server_queue_message_t));
	
	//core that out http server will run on
	config.core_id	= HTTP_SERVER_TASK_CORE_ID;
	
	//adjust default priority that is 1 less than wifi app task
	config.task_priority = HTTP_SERVER_TASK_PRIORITY;
	
	//increase the stack size
	config.stack_size  = HTTP_SERVER_TASK_STACK_SIZE;
	
	//increase our uri handler 
	config.max_uri_handlers = 20;
	
	//increase the timout limits
	config.recv_wait_timeout = 10;
	config.send_wait_timeout = 10;
	
	ESP_LOGI(TAG,"http_server_config: starting server on port: %d and in priority :%d", config.server_port, config.task_priority);
	
	
	//start the httpd_server
	if(httpd_start(&http_server_handle, &config)==ESP_OK){
		ESP_LOGI(TAG, "http_server_config: Registering URI_handlers");
		
		//register query handler
		httpd_uri_t jquery_js = {
			.uri = "/jquery-3.3.1.min.js",
			.method = HTTP_GET,
			.handler = http_server_jquery_handler,
			.user_ctx = NULL,
		};
		httpd_register_uri_handler(http_server_handle, &jquery_js);
		
		//register index_html handler
		httpd_uri_t index_html = {
			.uri = "/index.html",
			.method = HTTP_GET,
			.handler = http_server_index_html_handler,
			.user_ctx = NULL,
		};
		httpd_register_uri_handler(http_server_handle, &index_html);
		
		
		httpd_uri_t root = {
			.uri = "/",
			.method = HTTP_GET,
			.handler = http_server_index_html_handler, // reuse your index.html handler
			.user_ctx = NULL,
		};
		httpd_register_uri_handler(http_server_handle, &root);
		
				
		//register app_css handler
		httpd_uri_t app_css = {
			.uri = "/app.css",
			.method = HTTP_GET,
			.handler = http_server_app_css_handler,
			.user_ctx = NULL,
		};
		httpd_register_uri_handler(http_server_handle, &app_css);
		
		//register app_js handler
		httpd_uri_t app_js = {
			.uri = "/app.js",
			.method = HTTP_GET,
			.handler = http_server_app_js_handler,
			.user_ctx = NULL,
		};
		httpd_register_uri_handler(http_server_handle, &app_js);
		
		
		//register favicon_ico handler
		httpd_uri_t favicon_ico = {
			.uri = "/favicon.ico",
			.method = HTTP_GET,
			.handler = http_server_favicon_ico_handler,
			.user_ctx = NULL,
		};
		httpd_register_uri_handler(http_server_handle, &favicon_ico);
		
		
		//register ota update handler
		httpd_uri_t OTA_update = {
			.uri = "/OTAupdate",
			.method = HTTP_POST,
			.handler = http_server_OTA_update_handler,
			.user_ctx = NULL,
		};
		httpd_register_uri_handler(http_server_handle, &OTA_update);

		
		//register ota status handler
				httpd_uri_t OTA_status = {
					.uri = "/OTAstatus",
					.method = HTTP_POST,
					.handler = http_server_OTA_status_handler,
					.user_ctx = NULL,
				};
				httpd_register_uri_handler(http_server_handle, &OTA_status);
		
			//register dhtsensor json handler
			httpd_uri_t dht_sensor_json = {
					.uri = "/dhtSensor.json",
					.method = HTTP_GET,
					.handler = http_server_dht_sensor_json_handler,
					.user_ctx = NULL,
						};
				httpd_register_uri_handler(http_server_handle, &dht_sensor_json);

				//register wificonnect.json handler
			httpd_uri_t wifi_connect_json = {
					.uri = "/wifiConnect.json",
					.method = HTTP_POST,
					.handler = http_server_wifi_connect_json_handler,
					.user_ctx = NULL,
						};
				httpd_register_uri_handler(http_server_handle, &wifi_connect_json);

				//register wificonnect.json handler
			httpd_uri_t wifi_connect_status_json = {
					.uri = "/wifiConnectStatus",
					.method = HTTP_POST,
					.handler = http_server_wifi_connect_status_json_handler,
					.user_ctx = NULL,
						};
				httpd_register_uri_handler(http_server_handle, &wifi_connect_status_json);

				//register wificonnectinfo.json handler
			httpd_uri_t wifi_connect_info_json = {
					.uri = "/wifiConnectInfo.json",
					.method = HTTP_GET,
					.handler = http_server_wifi_get_connect_info_json_handler,
					.user_ctx = NULL,
						};
				httpd_register_uri_handler(http_server_handle, &wifi_connect_info_json);
				

				//register wifidisconnect.json handler
			httpd_uri_t wifi_disconnect_json = {
					.uri = "/disconnectWifi.json",
					.method = HTTP_GET,
					.handler = http_server_wifi_disconnect_json_handler,
					.user_ctx = NULL,
						};
				httpd_register_uri_handler(http_server_handle, &wifi_disconnect_json);

		return http_server_handle; 
	}
	return NULL;
 }
 
 
 
 //starts http_server
 void http_server_start(void){
	if(http_server_handle==NULL){
		http_server_handle = http_server_configure();
	}
 }


 //stops http_server
 void http_server_stop(void){
	if(http_server_handle){
			httpd_stop(http_server_handle);
			ESP_LOGI(TAG, "http_server_stop: http server stopping");
			http_server_handle=NULL;
		}
	
	if(task_http_server_monitor){
		vTaskDelete(task_http_server_monitor);
		ESP_LOGI(TAG, "task_http_server_monitor: task http server monitor deleted");
		task_http_server_monitor = NULL;
	}
 }
 
 
 BaseType_t http_server_monitor_send_message(http_server_message_e msgId){
	http_server_queue_message_t msg;
	msg.msgId = msgId;
	return xQueueSend(http_server_monitor_queue_handle, &msg, portMAX_DELAY); 
 }
 
 void http_server_fw_update_reset_callback(void *arg){
	ESP_LOGI(TAG,"http_server_fw_update_reset_callback: Timer Timed out...restarting device");
	esp_restart();
 }
