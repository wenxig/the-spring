#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

namespace {
constexpr char kTag[] = "the_spring";
}

extern "C" void app_main() {
  ESP_LOGI(kTag, "ESP32-P4 core firmware starting");
  vTaskDelay(pdMS_TO_TICKS(1000));
}
