idf_component_register( SRCS "pwm.c"
                        INCLUDE_DIRS "."
                        PRIV_REQUIRES esp_driver_ledc esp_driver_gpio)

