idf_component_register(
    SRCS "my_http_client.c" 
    INCLUDE_DIRS "."
    REQUIRES esp_netif esp-tls esp_http_client)

