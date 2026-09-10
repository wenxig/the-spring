#include "network_service.hpp"

#include "esp_log.h"

namespace {
constexpr char kTag[] = "network";
}

void spring::network::start() { ESP_LOGI(kTag, "network service ready"); }

spring::network::Response spring::network::get(std::string_view url) {
  if (url.empty()) return {.status = -1};
  return {.status = 501};
}

spring::network::Response spring::network::post(std::string_view url, std::string_view body) {
  if (url.empty() || body.empty()) return {.status = -1};
  return {.status = 501};
}
