#ifndef DHT22_H
#define DHT22_H

#define INIT_OK 2
#define DATA_OK 1
#define DHT22_TIMEOUT_ERROR -1
#define DHT22_CHECKSUM_ERROR -2
#define DHT22_INIT_ERROR -3

#include <stdio.h>
#include <math.h>
#include "driver/gpio.h"
#include "stdint.h"
#include "esp_rom_sys.h"
#include "esp_log.h" 
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void ERROR(int response);
int getSignalLevel(int timeout ,int state, int gpio_num);
int dht11_init(int gpio_num);
int dht11_read(int gpio_num,float *humidity, float *temperature);

#endif