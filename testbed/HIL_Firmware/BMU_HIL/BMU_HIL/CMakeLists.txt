idf_component_register(SRCS "processCAN.c" "userInit.c" "processCan.c" "UserInit.c"
                    INCLUDE_DIRS "inc"
                    REQUIRES common driver
                    REQUIRES esp_driver_ledc
                    REQUIRES esp_driver_rmt)
