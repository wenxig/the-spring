idf_component_register(
    SRCS "at_engine.cpp"
    INCLUDE_DIRS "include"
    REQUIRES esp_driver_uart esp_event esp_netif esp_http_client espressif__esp_modem
             espressif__esp_modem_usb_dte
             network_service http_transport
)
