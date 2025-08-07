#ifndef WIFI_H
#define WIFI_H
#include <string.h>
#include "esp_mac.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "esp_smartconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include "esp_system.h"
#include "lwip/ip4_addr.h"
#include "driver/gpio.h"


#define CONNECTED_BIT 1<<0
#define FAIL_BIT      1<<1
#define ESPTOUCH_DONE_BIT 1<<2

typedef enum {
    NONE=0,
    WIFI_MODE_SMARTCONFIG,
    WIFI_MODE_SOFTAP,
    WIFI_MODE_STATION,
} wifi_work_mode_t;

esp_err_t start_mdns_service(void);
void change_mode(const char *ssid, const char *password);
void wifi_set_mode(wifi_work_mode_t mode,const char *ssid, const char *password );
void wifi_disconnect(void);

#endif