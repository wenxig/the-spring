#include "esp_log.h"
#include "app_runtime.hpp"
#include "ui_core.hpp"
#include "display_service.hpp"
#include <cstdio>

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
  std::printf("SPRING_FRAME_BEGIN %lu %zu %08lx\n", static_cast<unsigned long>(++frame_id), bytes.size(), static_cast<unsigned long>(spring::display::frame_checksum()));
  for (const auto byte : bytes) std::printf("%02x", byte);
  std::printf("\nSPRING_FRAME_END %lu\n", static_cast<unsigned long>(frame_id));
  std::fflush(stdout);
}
