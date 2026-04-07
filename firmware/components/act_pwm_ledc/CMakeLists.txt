idf_component_register(SRCS "act_pwm_ledc.c"
                    INCLUDE_DIRS "include"
                    REQUIRES esp_driver_gpio esp_driver_ledc)
