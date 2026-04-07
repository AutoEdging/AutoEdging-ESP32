idf_component_register(SRCS "dglab_socket.c"
                    INCLUDE_DIRS "include"
                    REQUIRES nvs_flash esp_websocket_client esp_timer espressif__cjson)
