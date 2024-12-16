idf_component_register(SRCS "processCAN.c" "userInit.c" "dac.c" "pwm.c"
                    INCLUDE_DIRS "."
                    REQUIRES common driver)