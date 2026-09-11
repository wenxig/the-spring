#include "esp_log.h"
#include "app_runtime.hpp"
#include "ui_core.hpp"
#include "display_service.hpp"
#include <cstdio>
#include <cstdint>

namespace {
constexpr char kTag[] = "app_manager";
spring::app::Application* current = nullptr;
spring::ui::Router router;
}

void spring::app::start() {
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
  spring::ui::Frame frame;
  router.render(frame, {});
  spring::display::present(frame);
  ESP_LOGI(kTag, "ui route=%s dirty=%ux%u+%u+%u frame=%08lx", spring::ui::Router::title(router.route()), spring::display::pending_area().x, spring::display::pending_area().y, spring::display::pending_area().width, spring::display::pending_area().height, static_cast<unsigned long>(spring::display::frame_checksum()));
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
}
