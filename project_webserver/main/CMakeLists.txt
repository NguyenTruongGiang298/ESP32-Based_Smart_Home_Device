idf_component_register(
    SRCS "app_main.c"
    PRIV_REQUIRES http_server nvs_flash wifi http_client w2812b dht_11 pwm
    INCLUDE_DIRS "."
)


