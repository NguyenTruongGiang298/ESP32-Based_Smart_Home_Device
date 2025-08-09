#include "my_wifi.h"
#include "mdns.h"

/*================================TAG=================================================*/
static const char *TAG_MDNS="mdns";
static const char *TAG_MODE_2 = "wifi softAP";
static const char *TAG_MODE_1 = "wifi station";
static const char *TAG_MODE_3 = "smart_config";
static const char *TAG = "wifi";
/*================================PIN RESET===========================================*/
#define PIN_RESET 17
/*================================VARIABLE===========================================*/
/* WiFi event group */
static EventGroupHandle_t s_wifi_event_group = NULL;

/* Quản lý các instance event handler */
static esp_event_handler_instance_t instance_wifi_sta = NULL;
static esp_event_handler_instance_t instance_ip_event_sta = NULL;
static esp_event_handler_instance_t instance_sc_event = NULL;
static esp_event_handler_instance_t instance_continue_wifi_event = NULL;
static esp_event_handler_instance_t instance_continue_ip_event = NULL;

/* Trạng thái khởi tạo */
static bool event_loop_initialized = false;

/* Quản lý netif */
static esp_netif_t *esp_netif = NULL;

/* Quản lý trạng thái của netif_driver*/
static esp_err_t ret;

/* retry */
static int retry=0;

static void ensure_event_loop()
{
    if (!event_loop_initialized)
    {
        ESP_ERROR_CHECK(esp_netif_init());
        ESP_ERROR_CHECK(esp_event_loop_create_default());
        event_loop_initialized = true;
    }
}
static bool isSaved_config(void)
{
    wifi_config_t wifi_config;
    if ((esp_wifi_get_mode(NULL)) == ESP_ERR_WIFI_NOT_INIT)
    {
        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        ret = esp_wifi_init(&cfg);
    }
    ESP_ERROR_CHECK(esp_wifi_get_config(WIFI_IF_STA, &wifi_config));
    return wifi_config.sta.ssid[0] != 0x00;
}

static void wifi_erease_config(void)
{
    gpio_set_direction(PIN_RESET,GPIO_MODE_OUTPUT);
    gpio_set_level(PIN_RESET,1);
    wifi_config_t wifi_config = {0};
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_LOGI("wifi", "Erased saved WiFi config");
    vTaskDelay(pdMS_TO_TICKS(5000));
    gpio_set_level(PIN_RESET,0);
    esp_restart();
}

esp_err_t start_mdns_service()
{
    esp_err_t ret;
    // Khởi tạo mDNS
    ret = mdns_init();
    if(ret != ESP_OK) return ret;
    // hostname mDNS là "esp32"
    ret = mdns_hostname_set("esp32");
    if(ret != ESP_OK) return ret;
    // Name descrpition
    ret=mdns_instance_name_set("ESP32 Web Server");
    if(ret != ESP_OK) return ret;
    // http
    ret=mdns_service_add("ESP32 Web Server","http", "_tcp",80, NULL,0);
    if(ret != ESP_OK) return ret;
    ESP_LOGI(TAG_MDNS,"mdns completed");
    return ESP_OK;
}

static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if(arg == (void *)1)     
    {
        if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)  esp_wifi_connect(); 
        else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) 
        {
            if(retry<=5) 
            {
                esp_wifi_connect();
                ESP_LOGI(TAG_MODE_1, "connected WiFi %d... ", 5-retry);
                retry++;
            }
            else xEventGroupSetBits(s_wifi_event_group, FAIL_BIT);
        }
        if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) 
        {
            ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
            ESP_LOGI(TAG_MODE_1, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
            xEventGroupSetBits(s_wifi_event_group, CONNECTED_BIT);
            retry=0;
        }
    }
    else if (arg ==(void *)2)
    {
        if (event_id == WIFI_EVENT_AP_STACONNECTED) 
        {
            wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
            ESP_LOGI(TAG_MODE_2, "station "MACSTR" join, AID=%d", MAC2STR(event->mac), event->aid);
        } 
        else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) 
        {
            wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
            ESP_LOGI(TAG_MODE_2, "station "MACSTR" leave, AID=%d, reason=%d", MAC2STR(event->mac), event->aid, event->reason);
        }   
    }
    else if(arg == (void *)3)
    {
        if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) esp_wifi_connect();
        else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) 
        {
            if(retry<=5) 
            {
                esp_wifi_connect();
                ESP_LOGI(TAG_MODE_1, "connected WiFi %d... ", 5-retry);
                retry++;
            }
            else xEventGroupSetBits(s_wifi_event_group, FAIL_BIT);
        }
        else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) 
        {
            ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
            ESP_LOGI(TAG_MODE_3, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
            xEventGroupSetBits(s_wifi_event_group, CONNECTED_BIT);
        }
        else if (event_base == SC_EVENT && event_id == SC_EVENT_SCAN_DONE)  
            ESP_LOGI(TAG_MODE_3, "Scan done");
        else if (event_base == SC_EVENT && event_id == SC_EVENT_FOUND_CHANNEL) 
            ESP_LOGI(TAG_MODE_3, "Found channel");
        else if (event_base == SC_EVENT && event_id == SC_EVENT_GOT_SSID_PSWD)
        { 
            smartconfig_event_got_ssid_pswd_t *evt = (smartconfig_event_got_ssid_pswd_t *)event_data;
            wifi_config_t wifi_config;
            bzero(&wifi_config, sizeof(wifi_config_t));
            memcpy(wifi_config.sta.ssid, evt->ssid, sizeof(wifi_config.sta.ssid));
            memcpy(wifi_config.sta.password, evt->password, sizeof(wifi_config.sta.password));
            wifi_config.sta.bssid_set=evt->bssid_set;
            if(wifi_config.sta.bssid_set==true)
            {
                memcpy(wifi_config.sta.bssid, evt->bssid, sizeof(wifi_config.sta.bssid));
            }
            ESP_LOGI(TAG_MODE_3, "SSID:%s\n", wifi_config.sta.ssid);
            ESP_LOGI(TAG_MODE_3, "PASSWORD:%s\n", wifi_config.sta.password);
            ESP_ERROR_CHECK( esp_wifi_disconnect());
            ESP_ERROR_CHECK( esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
            esp_wifi_connect();
        }
        else if (event_base == SC_EVENT && event_id == SC_EVENT_SEND_ACK_DONE) 
        {
            xEventGroupSetBits(s_wifi_event_group, ESPTOUCH_DONE_BIT);
            retry=0;
        }
    }
}
static void wifi_init_mode_softAP(const char *my_ssid,const char *my_password)
{
    if (ret != ESP_OK) {
        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        ret = esp_wifi_init(&cfg);
    }
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        (void *)2,
                                                        NULL));
    wifi_config_t wifi_config = {
    .ap = 
        {
        .ssid_len = strlen(my_ssid),
        .channel = 1,
        .authmode = WIFI_AUTH_WPA_WPA2_PSK,
        .max_connection = 2,
        .pmf_cfg = {
            .required = false,
                },
        },
    };
    strncpy((char *)wifi_config.ap.ssid, my_ssid, sizeof(wifi_config.ap.ssid));
    strncpy((char *)wifi_config.ap.password, my_password, sizeof(wifi_config.ap.password));
    if (strlen(my_password) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_LOGI(TAG_MODE_2, "wifi_init_softap finished. SSID:%s password:%s", my_ssid, my_password);
}

static void wifi_init_mode_sta( const char *my_ssid, const char *my_password)
{
    if(ret != ESP_OK) 
    {
        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    }
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, (void *)1,&instance_wifi_sta));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,IP_EVENT_STA_GOT_IP,&wifi_event_handler,(void *)1,&instance_ip_event_sta));
    wifi_config_t wifi_config;
    bzero(&wifi_config, sizeof(wifi_config_t));
    strncpy((char *)wifi_config.sta.ssid, my_ssid, sizeof(wifi_config.sta.ssid));
    strncpy((char *)wifi_config.sta.password, my_password, sizeof(wifi_config.sta.password));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_LOGI(TAG_MODE_1, "wifi_init_sta finished. SSID:%s password:%s\n",my_ssid, my_password);
}

static void wifi_init_mode_smartcfg(void)
{
    if(ret != ESP_OK) 
    {
        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        ret = esp_wifi_init(&cfg) ;
    }
    ESP_ERROR_CHECK( esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, (void *)3,&instance_sc_event));
    ESP_ERROR_CHECK( esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, (void *)3, &instance_sc_event));
    ESP_ERROR_CHECK( esp_event_handler_instance_register(SC_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, (void *)3,&instance_sc_event ));
    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK( esp_wifi_start());

    ESP_ERROR_CHECK(esp_smartconfig_set_type(SC_TYPE_ESPTOUCH));
    smartconfig_start_config_t sc_cfg = SMARTCONFIG_START_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_smartconfig_start(&sc_cfg));
    ESP_LOGI(TAG_MODE_3, "SmartConfig started.");
}
void change_mode(const char *ssid, const char *password)
{
    wifi_disconnect(); 
    ensure_event_loop();
    if(s_wifi_event_group==NULL)    s_wifi_event_group = xEventGroupCreate();
    esp_netif=esp_netif_create_default_wifi_sta();
    wifi_init_mode_sta(ssid, password);
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                                                    CONNECTED_BIT | FAIL_BIT,
                                                    true, false, portMAX_DELAY);
    if (bits & CONNECTED_BIT) 
    {
        ESP_LOGI(TAG_MODE_1, "WiFi connected!");
    }
    else if (bits & FAIL_BIT) 
    {
        ESP_LOGI(TAG_MODE_1, "ERROR - Failed to connect WiFi!");
        wifi_erease_config();
    }
}
void wifi_set_mode(wifi_work_mode_t mode,const char *ssid, const char *password )
{
    if(s_wifi_event_group==NULL)    s_wifi_event_group = xEventGroupCreate();
    ensure_event_loop();
    if (mode == WIFI_MODE_SOFTAP) 
    { 
        if (esp_netif == NULL) esp_netif = esp_netif_create_default_wifi_ap();
    }
    else 
    {
        if (esp_netif == NULL) esp_netif = esp_netif_create_default_wifi_sta();
    }
    if (!isSaved_config())
    {
        switch (mode)
        {
            EventBits_t bits;
            case WIFI_MODE_SMARTCONFIG:
                wifi_init_mode_smartcfg();
                bits = xEventGroupWaitBits( s_wifi_event_group, FAIL_BIT | ESPTOUCH_DONE_BIT,
                                                        pdTRUE, pdFALSE, portMAX_DELAY);

                if (bits & ESPTOUCH_DONE_BIT) { ESP_LOGI(TAG_MODE_3, "SmartConfig completed");
                    esp_smartconfig_stop();
                }
                if (bits & FAIL_BIT) { ESP_LOGI(TAG_MODE_3, "ERROR!");
                    esp_smartconfig_stop();
                    wifi_erease_config();
                }
                break;
            case WIFI_MODE_STATION:
                wifi_init_mode_sta(ssid, password);
                bits = xEventGroupWaitBits(s_wifi_event_group, CONNECTED_BIT | FAIL_BIT,
                                                                    pdFALSE,
                                                                    pdTRUE,
                                                                    portMAX_DELAY);
                if (bits & CONNECTED_BIT) 
                {
                    ESP_LOGI(TAG_MODE_1, "connected WiFi successfully!\n");
                }
                ESP_LOGI(TAG_MODE_1, " Connect to WiFi. SSID: %s + Password: %s\n", ssid,password);
                if (bits & FAIL_BIT) 
                { 
                    ESP_LOGI(TAG_MODE_1, "ERROR!");
                    wifi_erease_config();
                }
                xEventGroupClearBits(s_wifi_event_group,CONNECTED_BIT);
                break;
            case WIFI_MODE_SOFTAP:
                wifi_init_mode_softAP(ssid, password);
                break;
            default:
                break;
        }   
    }
    else 
    {  
        wifi_mode_t current_mode;
        if(esp_netif == NULL || esp_wifi_get_mode(&current_mode) !=WIFI_MODE_STA) esp_netif = esp_netif_create_default_wifi_sta();
        ESP_ERROR_CHECK( esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, (void*)1, &instance_continue_wifi_event));
        ESP_ERROR_CHECK( esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, (void*)1, &instance_continue_ip_event));
        ESP_ERROR_CHECK(esp_wifi_start());
        EventBits_t uxBits = xEventGroupWaitBits(s_wifi_event_group,
                                                        CONNECTED_BIT | FAIL_BIT,
                                                        true, false, portMAX_DELAY);
        if (uxBits & CONNECTED_BIT) 
        {
            ESP_LOGI(TAG_MODE_1, "CONTINUE!");
        }
        else if (uxBits & FAIL_BIT) 
        {
            ESP_LOGI(TAG_MODE_1, "ERROR - Failed to connect WiFi!");
            wifi_erease_config();
        }
        if(start_mdns_service() == ESP_OK)
        {
           ESP_LOGI(TAG_MDNS,"initialize mDNS successfully");
        }
    }
}

void wifi_disconnect(void)
{
    xEventGroupClearBits(s_wifi_event_group, CONNECTED_BIT | FAIL_BIT | ESPTOUCH_DONE_BIT);
    wifi_mode_t mode;
    if (esp_wifi_get_mode(&mode) == ESP_OK && mode != WIFI_MODE_NULL)
    {
        esp_wifi_disconnect();
        esp_wifi_stop();
        esp_wifi_deinit();
    }
    if(instance_wifi_sta)
    {
        esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, instance_wifi_sta);
        ESP_LOGI(TAG, "Unregister \"instance_wifi_sta\" Handle");
        instance_wifi_sta = NULL;
    }
    if(instance_ip_event_sta)
    {
        esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, instance_ip_event_sta); 
        ESP_LOGI(TAG, "Unregister \"instance_ip_event_sta\" Handle");
        instance_ip_event_sta=NULL;
    }
    if(instance_continue_wifi_event) 
    {
        esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID,instance_continue_wifi_event);
        ESP_LOGI(TAG, "Unregister \"instance_continue_wifi_event\" Handle");
        instance_continue_wifi_event = NULL;
    }
    if(instance_continue_ip_event) 
    {
        esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, instance_continue_ip_event);
        ESP_LOGI(TAG, "Unregister \"instance_continue_ip_event\" Handle");
        instance_continue_ip_event = NULL;
    }
    if(instance_sc_event) 
    {
        esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, instance_sc_event);
        esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, instance_sc_event);
        esp_event_handler_instance_unregister(SC_EVENT, ESP_EVENT_ANY_ID, instance_sc_event);
        ESP_LOGI(TAG, "Unregister \"instance_sc_event\" Handle");
        instance_sc_event = NULL;
    }
    if (esp_netif != NULL)
    {
        ESP_ERROR_CHECK(esp_wifi_clear_default_wifi_driver_and_handlers(esp_netif));
        esp_netif_destroy_default_wifi(esp_netif);
        esp_netif = NULL;
    }
    if (event_loop_initialized)
    {
        esp_err_t err = esp_event_loop_delete_default();
        if (err == ESP_OK)
        {
            event_loop_initialized = false;
            ESP_LOGI(TAG, "Deleted default event loop");
        }
        else if (err == ESP_ERR_INVALID_STATE)
        {
            ESP_LOGW(TAG, "Default event loop still has registered handlers!");
            wifi_erease_config();
        }
        else
        {
            ESP_LOGE(TAG, "esp_event_loop_delete_default() failed: %s", esp_err_to_name(err));
        }
    }
    ret = -1;
    printf("Stop WIFI\n");
}
