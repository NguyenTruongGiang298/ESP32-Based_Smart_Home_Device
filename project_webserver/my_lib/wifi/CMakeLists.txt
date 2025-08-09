set(EXTRA_COMPONENT_DIRS ${CMAKE_CURRENT_LIST_DIR}/my_lib)
idf_component_register( SRCS "my_wifi.c"
                        INCLUDE_DIRS "."
                        REQUIRES esp_netif esp_wifi esp_event nvs_flash lwip mdns esp_driver_gpio
)
