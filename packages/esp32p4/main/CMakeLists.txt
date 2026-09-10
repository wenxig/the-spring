idf_component_register(SRCS "main.cpp" INCLUDE_DIRS "."
  REQUIRES app_runtime clock_service input_service modem_service display_service
           storage_service network_service wifi_service clock_app ota_service voice_service location_service)
