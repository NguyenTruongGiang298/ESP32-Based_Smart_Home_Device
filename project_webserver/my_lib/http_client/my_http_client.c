#include "my_http_client.h"

static const char *TAG = "HTTP_CLIENT";
static  char REQUEST[100]; 
void http_post_to_ubidots(const char *rsp)
{
    esp_http_client_config_t config = {
        .url= "http://industrial.api.ubidots.com/api/v1.6/devices/esp32_dashboard",
        .method = HTTP_METHOD_POST,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_header(client, "X-Auth-Token", "BBUS-jnuuVWJulAfMFumfXOa3But8jWKvEi");
    esp_http_client_set_post_field(client, rsp, strlen(rsp));
    esp_err_t err;
    err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "HTTP POST Status = %d, content_length = %"PRId64,
                esp_http_client_get_status_code(client),
                esp_http_client_get_content_length(client));
    } 
    else {
        ESP_LOGE(TAG, "HTTP POST request failed: %s", esp_err_to_name(err));
    }
    esp_http_client_cleanup(client);
}

void http_post_to_thingspeak(float temp, float hum)
{
    sprintf(REQUEST, "api_key=0PNL5TQYO7AW3WFV&field1=%.2f&field2=%.2f", hum, temp);

    esp_http_client_config_t config = {
        .url = "http://api.thingspeak.com/update",
        .method = HTTP_METHOD_POST,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);

    esp_http_client_set_header(client, "Content-Type", "application/x-www-form-urlencoded");
    esp_http_client_set_post_field(client, REQUEST, strlen(REQUEST));

    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "HTTP POST Status = %d, content_length = %"PRId64,
                 esp_http_client_get_status_code(client),
                 esp_http_client_get_content_length(client));
    } else {
        ESP_LOGE(TAG, "HTTP POST request failed: %s", esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);
}