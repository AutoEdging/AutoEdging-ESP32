idf_component_register(SRCS "wifi_service.c"
                    INCLUDE_DIRS "include"
                    REQUIRES esp_event esp_netif esp_wifi nvs_flash
                    PRIV_REQUIRES espressif__network_provisioning)
