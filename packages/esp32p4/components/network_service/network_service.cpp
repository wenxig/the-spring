#include "network_service.hpp"

#include "esp_log.h"
#include "esp_http_client.h"

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
  esp_http_client_config_t config{.url = url.data(), .timeout_ms = 15000};
  esp_http_client_handle_t client = esp_http_client_init(&config);
  if (client == nullptr) return {.status = -1};
  const auto result = esp_http_client_perform(client);
  Response response{.status = result == ESP_OK ? esp_http_client_get_status_code(client) : -1};
  esp_http_client_cleanup(client);
  return response;
}

spring::network::Response spring::network::post(std::string_view url, std::string_view body) {
  if (url.empty() || body.empty() || active_link() == Link::unavailable) return {.status = -1};
  esp_http_client_config_t config{.url = url.data(), .timeout_ms = 15000};
  esp_http_client_handle_t client = esp_http_client_init(&config);
  if (client == nullptr) return {.status = -1};
  esp_http_client_set_method(client, HTTP_METHOD_POST);
  esp_http_client_set_post_field(client, body.data(), static_cast<int>(body.size()));
  const auto result = esp_http_client_perform(client);
  Response response{.status = result == ESP_OK ? esp_http_client_get_status_code(client) : -1};
  esp_http_client_cleanup(client);
  return response;
}
