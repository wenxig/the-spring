idf_component_register(SRCS "app_manager.cpp" "weather_polling.cpp" INCLUDE_DIRS "include" REQUIRES ui_core display_service clock_service modem_service weather_service freertos)
