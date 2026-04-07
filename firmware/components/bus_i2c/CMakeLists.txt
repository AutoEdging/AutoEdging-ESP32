idf_component_register(SRCS "bus_i2c.c"
                    INCLUDE_DIRS "include"
                       REQUIRES esp_driver_gpio esp_driver_i2c)
