#include "esp_log.h"
#include "app_runtime.hpp"
#include "ui_core.hpp"
#include "declarative_ui.hpp"
#include "display_service.hpp"
#include "clock_service.hpp"
#include "at_engine.hpp"
#include "network_service.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include <array>
#include <algorithm>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <ctime>
#include <cmath>

namespace {
constexpr char kTag[] = "app_manager";
spring::app::Application* current = nullptr;
spring::ui::Router router;
SemaphoreHandle_t render_lock = nullptr;
EXT_RAM_BSS_ATTR spring::ui::Frame render_frame;
constexpr std::size_t kFramePacketSize = 4U + 2U + 4U + 4U + 4U + spring::display::kFrameBytes + 4U;
EXT_RAM_BSS_ATTR std::array<std::uint8_t, kFramePacketSize> frame_packet{};
}

void spring::app::start() {
  render_lock = xSemaphoreCreateMutex();
  ESP_LOGI(kTag, "application runtime ready");
}

bool spring::app::register_application(Application& application) {
  ESP_LOGI(kTag, "registered application: %s", application.name());
  if (current == nullptr) current = &application;
  return true;
}

bool spring::app::navigate_home() {
  if (current == nullptr) return false;
  router.dispatch(spring::ui::Event::home);
  current->on_event(0);
  render();
  return true;
}

bool spring::app::dispatch(spring::ui::Event event) {
  const auto accepted = router.dispatch(event);
  if (accepted) render();
  return accepted;
}

void spring::app::render() {
  if (render_lock == nullptr || xSemaphoreTake(render_lock, portMAX_DELAY) != pdTRUE) return;
  const auto clock = spring::clock::now();
  const auto modem = spring::modem::snapshot();
  const auto timestamp = static_cast<std::time_t>(clock.unix_seconds);
  const auto local = std::localtime(&timestamp);
  spring::ui::Snapshot snapshot{};
  if (local != nullptr) {
    snapshot.hour = static_cast<std::uint8_t>(local->tm_hour);
    snapshot.minute = static_cast<std::uint8_t>(local->tm_min);
  }
  snapshot.signal = modem.registered ? 4 : 0;
  snapshot.registered = modem.registered;
  snapshot.locating = !modem.location_valid;
  snapshot.in_call = modem.call_active;
  const auto local_weather = spring::network::weather();
  if (local != nullptr) {
    snapshot.year = static_cast<std::uint16_t>(local->tm_year + 1900);
    snapshot.month = static_cast<std::uint8_t>(local->tm_mon + 1);
    snapshot.day = static_cast<std::uint8_t>(local->tm_mday);
    snapshot.weekday = static_cast<std::uint8_t>(local->tm_wday);
    std::tm today = *local;
    today.tm_hour = 0;
    today.tm_min = 0;
    today.tm_sec = 0;
    auto exam = today;
    exam.tm_mon = 5;
    exam.tm_mday = 7;
    const auto today_time = std::mktime(&today);
    auto exam_time = std::mktime(&exam);
    if (exam_time <= today_time) {
      ++exam.tm_year;
      exam_time = std::mktime(&exam);
    }
    if (today_time != static_cast<std::time_t>(-1) && exam_time != static_cast<std::time_t>(-1)) {
      const auto remaining = std::difftime(exam_time, today_time) / (24.0 * 60.0 * 60.0);
      snapshot.countdown_days = static_cast<std::uint16_t>(std::max(0.0, std::ceil(remaining)));
    }
  }
  snapshot.weather_valid = local_weather.valid;
  snapshot.forecast_count = local_weather.count;
  for (std::size_t index{}; index < snapshot.forecast.size(); ++index) {
    snapshot.forecast[index].hour = local_weather.forecast[index].hour;
    snapshot.forecast[index].temperature_c = local_weather.forecast[index].temperature_c;
    snapshot.forecast[index].temperature_low_c = local_weather.forecast[index].temperature_low_c;
    snapshot.forecast[index].temperature_high_c = local_weather.forecast[index].temperature_high_c;
    std::strncpy(snapshot.forecast[index].description.data(), local_weather.forecast[index].description.data(),
                 snapshot.forecast[index].description.size() - 1);
    snapshot.forecast[index].description.back() = '\0';
  }
  #if CONFIG_SPRING_LVGL_DECLARATIVE_UI
  spring::ui::render_declarative(render_frame, snapshot, router);
  #else
  router.render(render_frame, snapshot);
  #endif
  spring::display::present(render_frame);
  ESP_LOGI(kTag, "ui route=%s dirty=%ux%u+%u+%u frame=%08lx", spring::ui::Router::title(router.route()), spring::display::pending_area().x, spring::display::pending_area().y, spring::display::pending_area().width, spring::display::pending_area().height, static_cast<unsigned long>(spring::display::frame_checksum()));
#if CONFIG_SPRING_DISPLAY_BUFFER_ONLY
  static std::uint32_t frame_id = 0;
  const auto bytes = spring::display::frame_bytes();
  constexpr std::uint32_t magic = 0x31504653U;
  constexpr std::uint16_t version = 1;
  const auto id = ++frame_id;
  const auto checksum = spring::display::frame_checksum();
  const auto length = static_cast<std::uint32_t>(bytes.size());
  std::size_t offset{};
  const auto append = [&](const auto value) {
    std::memcpy(frame_packet.data() + offset, &value, sizeof(value));
    offset += sizeof(value);
  };
  append(magic);
  append(version);
  append(id);
  append(length);
  append(checksum);
  std::memcpy(frame_packet.data() + offset, bytes.data(), bytes.size());
  offset += bytes.size();
  append(magic);
  std::fwrite(frame_packet.data(), offset, 1, stdout);
  std::fflush(stdout);
#endif
  if (spring::display::healthy()) spring::display::complete_refresh();
  xSemaphoreGive(render_lock);
}
