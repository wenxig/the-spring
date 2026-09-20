idf_component_register(SRCS "http_transport.cpp" INCLUDE_DIRS "include"
                       REQUIRES network_service esp_http_client esp_netif mbedtls esp-tls)
