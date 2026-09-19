idf_component_register(SRCS "wifi_service.cpp" INCLUDE_DIRS "include"
                       REQUIRES esp_hosted esp_wifi esp_event esp_netif nvs_flash network_service clock_service)
