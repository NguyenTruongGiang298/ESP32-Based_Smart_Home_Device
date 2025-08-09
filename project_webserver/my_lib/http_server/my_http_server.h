#ifndef HTTPS_APP_H
#define HTTPS_APP_H
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <esp_log.h>
#include <sys/param.h>
#include "esp_netif.h"
#include "esp_tls_crypto.h"
#include <esp_http_server.h>
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_tls.h"
#include "esp_check.h"

typedef void (*http_post_callback_t ) (char *data, long unsigned int len);
typedef void(*http_get_callback_t) (char *buf);

void start_webserver(void);
void stop_webserver(void);
void http_set_callback_for_get(void * cb);
void http_set_callback_for_buzzer(void * cb);
void http_set_callback_for_rgb(void * cb);
void http_set_wifi(void *cb);
#endif