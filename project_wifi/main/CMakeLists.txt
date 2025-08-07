idf_component_register(
    SRCS "app_main.c"
    PRIV_REQUIRES http_server dht_22 esp_wifi nvs_flash pwm esp_netif esp_event wifi http_client w2812b
    INCLUDE_DIRS "."
)


