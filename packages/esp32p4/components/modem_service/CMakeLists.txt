idf_component_register(
    SRCS "at_engine.cpp"
    INCLUDE_DIRS "include"
    REQUIRES esp_modem espressif__usb espressif__usb_host_cdc_acm
)
