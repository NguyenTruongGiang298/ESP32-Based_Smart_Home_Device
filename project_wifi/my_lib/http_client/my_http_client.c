#include "my_http_client.h"

static const char *TAG = "HTTP_CLIENT";

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