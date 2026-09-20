#include "wifi_service.hpp"

#include "clock_service.hpp"
#include "esp_crt_bundle.h"
#include "esp_err.h"
#include "esp_event.h"
#include "esp_hosted.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_sntp.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "http_transport.hpp"
#include "network_service.hpp"
#include "nvs_flash.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <new>
#include <string>
#include <string_view>
#include <vector>

namespace {
constexpr char kTag[] = "wifi";
constexpr char kPrimarySsid[] = "dlsflfl";
constexpr char kPrimaryPassword[] = "hb094263";
constexpr char kAxpPassword[] = "etiantian";
constexpr EventBits_t kConnected = BIT0;
constexpr EventBits_t kFailed = BIT1;
EventGroupHandle_t wifi_events = nullptr;
std::atomic_bool wifi_connected{false};

esp_netif_t* station_netif{};
class WifiTransport final : public spring::network::HttpTransport {
public:
  bool available() const noexcept override { return wifi_connected.load(); }
  spring::network::TransportResult
  perform(const spring::network::Request& request,
          spring::network::CancellationToken cancellation) override {
    return spring::http::perform(request, std::move(cancellation), station_netif);
  }
};

WifiTransport wifi_http_transport;

bool is_axp_network(std::string_view ssid) {
  constexpr std::string_view prefix{"axp-"};
  if (!ssid.starts_with(prefix) || ssid.size() == prefix.size())
    return false;
  return std::all_of(ssid.begin() + static_cast<std::ptrdiff_t>(prefix.size()), ssid.end(),
                     [](const char character) { return character >= '0' && character <= '9'; });
}

void on_wifi_event(void*, esp_event_base_t event_base, int32_t event_id, void* event_data) {
  if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
    wifi_connected.store(false);
    if (wifi_events != nullptr)
      xEventGroupSetBits(wifi_events, kFailed);
    const auto reason =
        event_data == nullptr
            ? -1
            : static_cast<int>(static_cast<wifi_event_sta_disconnected_t*>(event_data)->reason);
    ESP_LOGW(kTag, "Wi-Fi disconnected, reason=%d", reason);
    return;
  }
  if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
    wifi_connected.store(true);
    if (wifi_events != nullptr)
      xEventGroupSetBits(wifi_events, kConnected);
    const auto* event = static_cast<const ip_event_got_ip_t*>(event_data);
    ESP_LOGI(kTag, "Wi-Fi connected, IPv4=" IPSTR, IP2STR(&event->ip_info.ip));
  }
}

bool connect_to(std::string_view ssid, std::string_view password) {
  wifi_config_t config{};
  std::copy_n(ssid.data(), std::min(ssid.size(), sizeof(config.sta.ssid) - 1),
              reinterpret_cast<char*>(config.sta.ssid));
  std::copy_n(password.data(), std::min(password.size(), sizeof(config.sta.password) - 1),
              reinterpret_cast<char*>(config.sta.password));
  if (esp_wifi_set_config(WIFI_IF_STA, &config) != ESP_OK || esp_wifi_connect() != ESP_OK)
    return false;
  const auto bits = xEventGroupWaitBits(wifi_events, kConnected | kFailed, pdTRUE, pdFALSE,
                                        pdMS_TO_TICKS(15'000));
  return (bits & kConnected) != 0;
}

bool scan_for_axp(std::string& ssid) {
  if (esp_wifi_scan_start(nullptr, true) != ESP_OK)
    return false;
  std::uint16_t count = 0;
  if (esp_wifi_scan_get_ap_num(&count) != ESP_OK || count == 0)
    return false;
  const auto records = std::min<std::uint16_t>(count, 32);
  auto* aps = new (std::nothrow) wifi_ap_record_t[records];
  if (aps == nullptr)
    return false;
  auto requested = records;
  const auto result = esp_wifi_scan_get_ap_records(&requested, aps);
  if (result == ESP_OK) {
    for (std::uint16_t index = 0; index < requested; ++index) {
      const std::string_view candidate{reinterpret_cast<const char*>(aps[index].ssid)};
      if (is_axp_network(candidate)) {
        ssid.assign(candidate);
        delete[] aps;
        return true;
      }
    }
  }
  delete[] aps;
  return false;
}

void sync_clock_from_wifi() {
  setenv("TZ", "CST-8", 1);
  tzset();
  esp_sntp_setoperatingmode(ESP_SNTP_OPMODE_POLL);
  esp_sntp_setservername(0, "ntp.aliyun.com");
  esp_sntp_setservername(1, "time1.cloud.tencent.com");
  esp_sntp_init();
  for (auto attempt = 0; attempt < 12; ++attempt) {
    vTaskDelay(pdMS_TO_TICKS(1'000));
    const auto now = std::time(nullptr);
    if (now > 1'700'000'000) {
      spring::clock::set_unix_seconds(static_cast<std::int64_t>(now));
      ESP_LOGI(kTag, "clock synchronized from Wi-Fi NTP");
      return;
    }
  }
  ESP_LOGW(kTag, "Wi-Fi connected but NTP did not complete");
}

void wifi_task(void*) {
  auto nvs_result = nvs_flash_init();
  if (nvs_result == ESP_ERR_NVS_NO_FREE_PAGES || nvs_result == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    nvs_result = nvs_flash_erase();
    if (nvs_result == ESP_OK)
      nvs_result = nvs_flash_init();
  }
  if (nvs_result != ESP_OK || esp_netif_init() != ESP_OK) {
    ESP_LOGE(kTag, "Wi-Fi prerequisites failed: nvs=%s", esp_err_to_name(nvs_result));
    vTaskDelete(nullptr);
    return;
  }
  const auto event_result = esp_event_loop_create_default();
  if (event_result != ESP_OK && event_result != ESP_ERR_INVALID_STATE) {
    ESP_LOGE(kTag, "default event loop failed: %s", esp_err_to_name(event_result));
    vTaskDelete(nullptr);
    return;
  }
  if (!spring::wifi::prepare_transport()) {
    ESP_LOGE(kTag, "ESP-Hosted C6 transport unavailable");
    vTaskDelete(nullptr);
    return;
  }
  station_netif = esp_netif_create_default_wifi_sta();
  if (station_netif == nullptr) {
    ESP_LOGE(kTag, "unable to create Wi-Fi STA netif");
    vTaskDelete(nullptr);
    return;
  }
  wifi_init_config_t init_config = WIFI_INIT_CONFIG_DEFAULT();
  if (esp_wifi_init(&init_config) != ESP_OK ||
      esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &on_wifi_event, nullptr) != ESP_OK ||
      esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &on_wifi_event, nullptr) !=
          ESP_OK ||
      esp_wifi_set_storage(WIFI_STORAGE_RAM) != ESP_OK ||
      esp_wifi_set_mode(WIFI_MODE_STA) != ESP_OK || esp_wifi_start() != ESP_OK) {
    ESP_LOGE(kTag, "ESP-Hosted Wi-Fi initialization failed");
    vTaskDelete(nullptr);
    return;
  }

  ESP_LOGI(kTag, "trying configured Wi-Fi: %s", kPrimarySsid);
  auto connected = connect_to(kPrimarySsid, kPrimaryPassword);
  std::string discovered_ssid;
  if (!connected && scan_for_axp(discovered_ssid)) {
    ESP_LOGI(kTag, "trying discovered Wi-Fi: %s", discovered_ssid.c_str());
    connected = connect_to(discovered_ssid, kAxpPassword);
  }
  if (connected) {
    sync_clock_from_wifi();
  } else {
    wifi_connected.store(false);
    ESP_LOGW(kTag, "no configured Wi-Fi network connected; cellular fallback enabled");
  }
  vTaskDelete(nullptr);
}
} // namespace

bool spring::wifi::prepare_transport() {
  return esp_hosted_init() == ESP_OK && esp_hosted_connect_to_slave() == ESP_OK;
}

void spring::wifi::start() {
  wifi_events = xEventGroupCreate();
  if (wifi_events == nullptr) {
    ESP_LOGE(kTag, "unable to allocate Wi-Fi event group");
    return;
  }
  wifi_connected.store(false);
  xTaskCreate(wifi_task, "wifi_connect", 8192, nullptr, 5, nullptr);
}

void spring::wifi::set_connected(bool connected) { wifi_connected.store(connected); }

spring::network::HttpTransport& spring::wifi::transport() { return wifi_http_transport; }
