#include "wifi_service.hpp"
#include "network_service.hpp"
#include "esp_log.h"

namespace {
constexpr char kTag[] = "wifi";
}

void spring::wifi::start() {
  ESP_LOGI(kTag, "Wi-Fi service ready");
  spring::network::set_link_available(spring::network::Link::wifi, false);
}

void spring::wifi::set_connected(bool connected) {
  spring::network::set_link_available(spring::network::Link::wifi, connected);
}
