#include "esp_log.h"
#include "app_runtime.hpp"
#include "ui_core.hpp"
#include "display_service.hpp"
#include "clock_service.hpp"
#include "at_engine.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include <cstdio>
#include <cstdint>
#include <ctime>

namespace {
constexpr char kTag[] = "app_manager";
spring::app::Application* current = nullptr;
spring::ui::Router router;
SemaphoreHandle_t render_lock = nullptr;
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
  spring::ui::Frame frame;
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
  router.render(frame, snapshot);
  spring::display::present(frame);
  ESP_LOGI(kTag, "ui route=%s dirty=%ux%u+%u+%u frame=%08lx", spring::ui::Router::title(router.route()), spring::display::pending_area().x, spring::display::pending_area().y, spring::display::pending_area().width, spring::display::pending_area().height, static_cast<unsigned long>(spring::display::frame_checksum()));
#if CONFIG_SPRING_DISPLAY_BUFFER_ONLY
  static std::uint32_t frame_id = 0;
  const auto bytes = spring::display::frame_bytes();
  constexpr std::uint32_t magic = 0x31504653U;
  constexpr std::uint16_t version = 1;
  const auto id = ++frame_id;
  const auto checksum = spring::display::frame_checksum();
  std::fwrite(&magic, sizeof(magic), 1, stdout);
  std::fwrite(&version, sizeof(version), 1, stdout);
  std::fwrite(&id, sizeof(id), 1, stdout);
  const auto length = static_cast<std::uint32_t>(bytes.size());
  std::fwrite(&length, sizeof(length), 1, stdout);
  std::fwrite(&checksum, sizeof(checksum), 1, stdout);
  std::fwrite(bytes.data(), 1, bytes.size(), stdout);
  std::fwrite(&magic, sizeof(magic), 1, stdout);
  std::fflush(stdout);
#endif
  spring::display::complete_refresh();
  xSemaphoreGive(render_lock);
}
