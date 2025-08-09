#include "my_http_server.h"


static const char *TAG = "https";
static httpd_handle_t server = NULL;
extern const uint8_t _binary_webserver_html_start[] asm("_binary_webserver_html_start");
extern const uint8_t _binary_webserver_html_end[]   asm("_binary_webserver_html_end");

extern const uint8_t _binary_dashboard_html_start[] asm("_binary_dashboard_html_start");
extern const uint8_t _binary_dashboard_html_end[]   asm("_binary_dashboard_html_end");

// function pointer
static http_get_callback_t  http_get_callback=NULL;
static http_post_callback_t http_rgb_callback=NULL;
static http_post_callback_t http_wifi_change=NULL;
static http_post_callback_t http_buzzer_callback=NULL;

/* An HTTP GET handler */
static esp_err_t get_WiFi_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req,"text/html");
    httpd_resp_send(req, (const char *)_binary_webserver_html_start,_binary_webserver_html_end - _binary_webserver_html_start);
    return ESP_OK;
}

static const httpd_uri_t get_wifi_for_webserver = {
    .uri       = "/esp32_wifi",
    .method    = HTTP_GET,
    .handler   = get_WiFi_handler,
    .user_ctx  = "Using dht11!"
};

static esp_err_t get_Dashboard_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req,"text/html");
    httpd_resp_send(req, (const char *)_binary_dashboard_html_start,_binary_dashboard_html_end - _binary_dashboard_html_start);
    return ESP_OK;
}

static const httpd_uri_t get_dashboard_for_webserver ={
    .uri       = "/esp32_dashboard",
    .method    = HTTP_GET,
    .handler   = get_Dashboard_handler,
    .user_ctx  = "convert pages"
};

/*GET reload data 1 per second */
static esp_err_t get_reload_handler(httpd_req_t *req)
{
    char resp_str[100]; 
    if(http_get_callback) 
    {
        http_get_callback(resp_str);
        httpd_resp_send(req, resp_str, strlen(resp_str));
    }
    else 
    {
        ESP_LOGE(TAG, "http_get_callback is NULL!");
    }
    return ESP_OK;
}
static const httpd_uri_t get_reload_data = {
    .uri       = "/reload_data",
    .method    = HTTP_GET,
    .handler   = get_reload_handler,
    .user_ctx  = "reload!"
};
static esp_err_t post_handler(httpd_req_t *req)
{
    char buf[100];
    httpd_req_recv(req, buf, req->content_len);
    if(strstr(req->uri,"rgb"))
        http_rgb_callback(buf,req->content_len);
    if(strstr(req->uri,"alert"))
        http_buzzer_callback(buf,req->content_len);
    else if (strstr(req->uri, "wifiinfo"))
    {
        http_wifi_change(buf,req->content_len);
    }
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

static const httpd_uri_t post_data = {
    .uri       = "/post",
    .method    = HTTP_POST,
    .handler   = post_handler,
    .user_ctx  = NULL
};

esp_err_t http_404_error_handler(httpd_req_t *req, httpd_err_code_t err)
{
    if (strcmp("/post", req->uri) == 0) {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "/esp32_dashboard URI is not available");
        return ESP_OK;
    } 
    httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Some 404 error message");
    return ESP_FAIL;
}


void start_webserver(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.lru_purge_enable = true;
    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        // Set URI handlers
        ESP_LOGI(TAG, "Registering URI handlers");
        httpd_register_uri_handler(server, &get_wifi_for_webserver);
        httpd_register_uri_handler(server, &get_dashboard_for_webserver);
        httpd_register_uri_handler(server, &get_reload_data);
        httpd_register_uri_handler(server, &post_data);
        httpd_register_err_handler(server,HTTPD_404_NOT_FOUND,http_404_error_handler);
    }
    else ESP_LOGI(TAG, "Error starting server!");
}

void stop_webserver(void)
{
    if (server != NULL) 
    {
        httpd_stop(server);
        server = NULL;
    }
    
}

void http_set_callback_for_get(void * cb)
{
    http_get_callback = cb;
}
void http_set_callback_for_rgb(void * cb)
{
    http_rgb_callback = cb;
}
void http_set_callback_for_buzzer(void * cb)
{
    http_buzzer_callback = cb;
}
void http_set_wifi(void *cb)
{
    http_wifi_change=cb;
}