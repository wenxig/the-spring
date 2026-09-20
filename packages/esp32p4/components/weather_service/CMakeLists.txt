idf_component_register(SRCS "weather_service.cpp" INCLUDE_DIRS "include"
                       REQUIRES network_service location_service cjson freertos)
