/*
 * tasks_common.h
 *
 *  Created on: May 23, 2025
 *      Author: ASUS
 */

#ifndef MAIN_TASKS_COMMON_H_
#define MAIN_TASKS_COMMON_H_


//wifi application settings
#define WIFI_APP_TASK_STACK_SIZE		4096
#define WIFI_APP_TASK_PRIORITY			5
#define WIFI_APP_TASK_CORE_ID			0


//http_server task settings
#define HTTP_SERVER_TASK_STACK_SIZE		8192
#define HTTP_SERVER_TASK_PRIORITY		4
#define HTTP_SERVER_TASK_CORE_ID		0


//http_server monitor task settings
#define HTTP_SERVER_MONITOR_STACK_SIZE	4096
#define HTTP_SERVER_MONITOR_PRIORITY	3
#define HTTP_SERVER_MONITOR_CORE_ID		0

//DHT22 task settings
#define DHT22_TASK_STACK_SIZE			4096
#define DHT22_TASK_PRIORITY				5
#define DHT22_TASK_CORE_ID				1


#endif /* MAIN_TASKS_COMMON_H_ */
