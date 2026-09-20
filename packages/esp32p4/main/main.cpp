#include "app_runtime.hpp"
#include "at_engine.hpp"
#include "clock_app.hpp"
#include "clock_service.hpp"
#include "display_service.hpp"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "input_service.hpp"
#include "network_service.hpp"
#include "ota_service.hpp"
#include "power_service.hpp"
#include "sdkconfig.h"
#include "storage_service.hpp"
#include "ui_core.hpp"
#include "wifi_service.hpp"
#include "weather_service.hpp"

namespace {
void interaction_task(void*) {
  while (true) {
    spring::input::Key key{};
    if (!spring::input::next(key, 1000))
      continue;
    if (spring::power::state() == spring::power::State::sleeping) {
      spring::power::wake();
      spring::app::render();
      continue;
    }
    if (key == spring::input::Key::sleep) {
      spring::app::dispatch(spring::ui::Event::sleep);
      spring::power::request_sleep();
      continue;
    }
    if (key == spring::input::Key::home)
      spring::app::navigate_home();
    else
      spring::app::dispatch(static_cast<spring::ui::Event>(key));
  }
}

void clock_refresh_task(void*) {
  auto last_minute = std::uint8_t{0xFF};
  auto last_modem_revision = std::uint64_t{};
  auto last_weather_revision = std::uint64_t{};
  while (true) {
    vTaskDelay(pdMS_TO_TICKS(1'000));
    const auto minute = static_cast<std::uint8_t>((spring::clock::now().unix_seconds / 60) % 60);
    const auto modem_revision = spring::modem::snapshot().revision;
    const auto weather_revision = spring::weather::snapshot().revision;
    if (spring::power::state() == spring::power::State::active &&
        (minute != last_minute || modem_revision != last_modem_revision ||
         weather_revision != last_weather_revision)) {
      last_minute = minute;
      last_modem_revision = modem_revision;
      last_weather_revision = weather_revision;
      spring::app::render();
    }
  }
}
} // namespace

namespace {
constexpr char kTag[] = "the_spring";
}

extern "C" void app_main() {
  ESP_LOGI(kTag, "ESP32-P4 core firmware starting");
  spring::clock::start();
  spring::input::start();
  spring::app::start();
#if CONFIG_SPRING_DISPLAY_BUFFER_ONLY
  ESP_LOGI(kTag, "display mode: buffer-only (P6 may be disconnected)");
  spring::display::start(spring::display::Backend::buffer_only);
#else
  ESP_LOGI(kTag, "display mode: epaper (P6 must be connected)");
  spring::display::start(spring::display::Backend::epaper);
#endif
  spring::modem::start();
  if (spring::wifi::prepare_transport()) {
    spring::storage::mount_sdcard();
  } else {
    ESP_LOGE(kTag, "Shared SDMMC controller unavailable; SD card mount skipped");
  }
  spring::storage::ui_assets_ready();
  spring::wifi::start();
  spring::network::register_transport(spring::network::Link::wifi, spring::wifi::transport());
  spring::network::register_transport(spring::network::Link::cellular, spring::modem::transport());
  spring::network::start();
  spring::weather::start();
  spring::app::start_weather_polling();
  spring::power::start();
  xTaskCreate(interaction_task, "interaction", 24576, nullptr, 5, nullptr);
  xTaskCreate(clock_refresh_task, "clock_refresh", 24576, nullptr, 4, nullptr);
  spring::clock_app::register_app();
  spring::app::render();
  spring::ota::mark_boot_valid();
}
