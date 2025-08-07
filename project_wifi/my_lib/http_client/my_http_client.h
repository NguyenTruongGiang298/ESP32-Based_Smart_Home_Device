#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H
#include <string.h>
#include <stdlib.h>
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_system.h"
#include "esp_http_client.h"

void http_post_to_ubidots(const char *rsp);

#endif
