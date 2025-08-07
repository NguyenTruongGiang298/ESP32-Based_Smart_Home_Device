#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_mac.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "driver/gpio.h"
#include "dht11.h"
#include "pwm.h"
#include "my_wifi.h"
#include "esp_netif.h"
#include "my_http_client.h"
#include "my_http_server.h"
#include "w2812b.h"

//==========================================PWM=====================================//
#define PWM_PIN HIGHSPEED_CH3_GPIO
ledc_channel_config_t ch;

//===============================================SETUP DHT22==========================//
#define DHT11_PIN 4
float current_temp=0, current_hum=0;
TaskHandle_t dht_task=NULL;
char resp_str[100];

//===============================================SETUP BUTTON==========================//
#define BUTTON_PIN 1
int turnOn=0;

//===============================================W2812B================================//
#define W2812_PIN 5
#define W2812_FREQ 10000000
#define MAX_LEDS 12
typedef struct
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
}Color;

//============================SET UP WIFI===================================//
const char* ssid=  "ESP32_WIFI";
const char* password="12345678";

//=============================TAG==========================================//
const char *TAG =     "WIFI";
const char *TAG_POST = "POST";
const char *TAG_GET = "GET";

void slider_handler(char *data, long unsigned int len)
{
    ESP_LOGI(TAG_POST, "=========== RECEIVED DATA ==========");
    ESP_LOGI(TAG_POST, "%s", data);
    ESP_LOGI(TAG_POST, "====================================");
    int pwm_val=atoi(data); 
    if(turnOn)
    {
        if (pwm_val<=100)
        {  
            uint32_t newduty =(uint32_t)(pwm_val*(1<<13)/100);
            update_duty(&ch,newduty);
            ch.duty=newduty;
            printf("duty: %ld\n",ch.duty);
        }
    }
    
}
void button_handler(char *data, long unsigned int len)
{
    ESP_LOGI(TAG_POST, "=========== RECEIVED DATA ==========");
    ESP_LOGI(TAG_POST, "%s", data);
    ESP_LOGI(TAG_POST, "====================================");
    if(strcmp(data,"1")==0)
    {
       turnOn=1;
    }
    else if(strcmp(data,"0")==0)
    {
        turnOn=0;
    }
}

uint8_t char_to_hex(char **c)
{
    uint8_t rgb[2];
    uint8_t cnt=0;
    while(cnt <2)
    {
        if (**c >= '0' && **c <= '9') rgb[cnt]= **c - '0';      
        else if (**c >= 'A' && **c <= 'F') rgb[cnt]= **c - 'A' + 10; 
        else if (**c >= 'a' && **c <= 'f') rgb[cnt]= **c- 'a' + 10; 
        (*c)++;
        cnt++;
    }
    return rgb[0] <<4 | rgb[1]; 
}
void w2812b_handler(char *data, long unsigned int len)
{
    ESP_LOGI(TAG_POST, "=========== RECEIVED DATA ==========");
    ESP_LOGI(TAG_POST, "%s", data);
    ESP_LOGI(TAG_POST, "====================================");
    uint8_t *rx_data=(uint8_t *)malloc(1);
    char *token_color = strtok(data, "/");
    char *token_number_led = strtok(NULL, "/");
     if (!token_color || !token_number_led) {
        ESP_LOGE(TAG_POST, "Invalid payload, expect <hexcolor>/<count>");
        free(rx_data);
        rx_data=NULL;
        return;
    }
    int number_of_led = atoi(token_number_led);
    if (number_of_led <= 0 || number_of_led > MAX_LEDS) {
        ESP_LOGE(TAG_POST, "Invalid LED count: %d", number_of_led);
        free(rx_data);
        rx_data=NULL;
        return;
    }
    Color rgb;
    rgb.r = char_to_hex(&token_color);
    rgb.g = char_to_hex(&token_color);
    rgb.b = char_to_hex(&token_color);
    ESP_LOGI(TAG_POST, "0x%x-0x%x-0x%x",rgb.r,rgb.g, rgb.b);

    rx_data = (uint8_t *)realloc(rx_data,number_of_led*3);
    for(int i=0;i<number_of_led;i++)
    {
        rx_data[i*3]=rgb.g;
        rx_data[i*3+1] =rgb.r;
        rx_data[i*3+2]=rgb.b;
    }
    w2812b_setData(rx_data,number_of_led);
    vTaskDelay(pdMS_TO_TICKS(10));
    w2812b_Tx_Reset();
    if(rx_data)
    {
        free(rx_data);
        rx_data=NULL;
    }
}
void get_reload_data(char *buf)
{
    sprintf(buf,"{\"temperature\": \"%.2f\",\"humidity\": \"%.2f\"}",current_temp, current_hum); 
}

void change_wifi(char *data, long unsigned int len)
{   
    vTaskSuspend(dht_task);
    char ssid[50] = {0};
    char password[50] = {0};
    char *token = strtok(data, "/");
    if (token != NULL) 
    {
        strncpy(ssid, token, sizeof(ssid) - 1);
        token = strtok(NULL, "/");
        if (token != NULL) 
        {
            strncpy(password, token, sizeof(password) - 1);
        } 
    }
    change_mode(ssid, password);
    vTaskDelay(pdMS_TO_TICKS(1000));
    start_mdns_service();
    vTaskResume(dht_task);
}
void xtask(void *pvParameter)
{
    float hum = 0;
    float temp = 0;
    float last_hum = 0;
    float last_temp = 0;
    static int cnt = 0;

    while (1)
    {
        if (dht11_init(DHT11_PIN) != INIT_OK)
            printf("Failed to init DHT11\n");

        int result = dht11_read(DHT11_PIN, &hum, &temp);
        if (result == DATA_OK)
        {
            cnt++;
            if (cnt >= 5 && last_hum != 0.0f && last_temp != 0.0f)
            {
                float hum_diff = fabs(hum - last_hum) / last_hum * 100.0f;
                float temp_diff = fabs(temp - last_temp);

                if (hum >= 20.0f && hum <= 90.0f && hum_diff < 0.5f)
                    current_hum = hum;


                if (temp >= 0.0f && temp <= 50.0f && temp_diff < 0.5f)
                    {
                       current_temp = temp;
                       if(temp > 30.0f) turnOn=1;
                       else turnOn=0;
                    }
                cnt = 5;
                last_hum = current_hum;
                last_temp = current_temp;
            }
            else if (cnt < 5)
            {
                last_hum = hum;
                last_temp = temp;
            }
        }
        snprintf(resp_str, sizeof(resp_str),
                 "{\"temperature\": \"%.2f\",\"humidity\": \"%.2f\"}",
                 current_temp, current_hum);

        http_post_to_ubidots(resp_str);
        ERROR(result);
        ESP_LOGI(TAG_GET, "humidity: %.2f, temperature: %.2f", hum, temp);

        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

void app_main(void)
{
    int ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret); 
    wifi_set_mode(WIFI_MODE_SOFTAP,ssid,password);
    start_webserver();
    http_set_wifi(change_wifi);
    http_set_callback_for_button(button_handler);
    http_set_callback_for_rgb(w2812b_handler);
    http_set_callback_for_slider(slider_handler);
    http_set_callback_for_get(get_reload_data);
    w2812b_init(W2812_PIN,W2812_FREQ);
    xTaskCreate(xtask, "dht22_task", 4096, NULL, 5, &dht_task);
}
