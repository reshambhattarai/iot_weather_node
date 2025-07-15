/*
 * rgb_led.h
 *
 *  Created on: May 23, 2025
 *      Author: ASUS
 */

#ifndef MAIN_RGB_LED_H_
#define MAIN_RGB_LED_H_


//rgb led gpio config

#define RGB_LED_RED_GPIO 		21
#define RGB_LED_GREEN_GPIO 		22
#define RGB_LED_BLUE_GPIO 		23

//rgb color mix channels

#define RGB_LED_CHANNEL_NUM			3

//rgb led configuration

typedef struct {
	int channel;
	int gpio;
	int mode;
	int timer_index;
} led_info_t;

extern 	led_info_t	ledc_ch[RGB_LED_CHANNEL_NUM];

/*
color to indicate wifi status
*/
void rgb_led_wifi_app_started(void);


/*
color to indicate http server started
*/
void rgb_led_http_server_started(void);


/*
color to indicate ESP 32 has connected to access point
*/
void rgb_led_wifi_connected(void);


#endif /* MAIN_RGB_LED_H_ */
