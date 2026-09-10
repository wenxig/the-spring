#include "network_service.hpp"

#include "esp_log.h"

namespace {
constexpr char kTag[] = "network";
bool wifi_available = false;
bool cellular_available = false;
}

void spring::network::start() { ESP_LOGI(kTag, "network service ready"); }

void spring::network::set_link_available(Link link, bool available) {
  if (link == Link::wifi) wifi_available = available;
  if (link == Link::cellular) cellular_available = available;
}

spring::network::Link spring::network::active_link() {
  if (wifi_available) return Link::wifi;
  if (cellular_available) return Link::cellular;
  return Link::unavailable;
}

spring::network::Response spring::network::get(std::string_view url) {
  if (url.empty() || active_link() == Link::unavailable) return {.status = -1};
  return {.status = 501};
}

spring::network::Response spring::network::post(std::string_view url, std::string_view body) {
  if (url.empty() || body.empty() || active_link() == Link::unavailable) return {.status = -1};
  return {.status = 501};
}
