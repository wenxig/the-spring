#include "network_service.hpp"

#include "esp_log.h"
#include "at_engine.hpp"
#include "clock_service.hpp"
#include "location_service.hpp"
#include "cJSON.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "sdkconfig.h"
#include <atomic>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <string>

namespace {
constexpr char kTag[] = "network";
std::atomic_bool wifi_available{false};
std::atomic_bool cellular_available{false};
spring::network::WeatherSnapshot weather_state{};
SemaphoreHandle_t weather_lock = nullptr;

void install_fallback_weather() {
  spring::network::WeatherSnapshot fallback{};
  fallback.valid = true;
  fallback.count = 3;
  fallback.forecast = {{{.hour = 18, .temperature_c = 26, .description = "晴"},
                        {.hour = 21, .temperature_c = 24, .description = "多云"},
                        {.hour = 0, .temperature_c = 22, .description = "小雨"}}};
  fallback.revision = weather_state.revision + 1;
  if (weather_lock == nullptr || xSemaphoreTake(weather_lock, pdMS_TO_TICKS(100)) == pdTRUE) {
    weather_state = fallback;
    if (weather_lock != nullptr) xSemaphoreGive(weather_lock);
  }
}

bool query_response(std::string_view command, std::string& response, std::uint32_t timeout_ms) {
  return spring::modem::execute_capture(command, timeout_ms, response) == spring::modem::Result::ok;
}

bool parse_cclk(std::string_view response, std::int64_t& unix_seconds) {
  const auto start = response.find("+CCLK:");
  if (start == std::string_view::npos) return false;
  int year{}, month{}, day{}, hour{}, minute{}, second{}, zone{};
  char sign{'+'};
  const auto text = std::string{response.substr(start)};
  if (std::sscanf(text.c_str(), "+CCLK: \"%d/%d/%d,%d:%d:%d%c%d", &year, &month, &day,
                  &hour, &minute, &second, &sign, &zone) != 8) return false;
  year += year < 70 ? 2000 : 1900;
  const auto y = static_cast<std::int64_t>(year);
  const auto adjusted_year = y - (month <= 2 ? 1 : 0);
  const auto era = (adjusted_year >= 0 ? adjusted_year : adjusted_year - 399) / 400;
  const auto year_of_era = adjusted_year - era * 400;
  const auto month_prime = static_cast<std::int64_t>(month + (month > 2 ? -3 : 9));
  const auto day_of_year = (153 * month_prime + 2) / 5 + day - 1;
  const auto day_of_era = year_of_era * 365 + year_of_era / 4 - year_of_era / 100 + day_of_year;
  const auto days = era * 146097 + day_of_era - 719468;
  const auto local_seconds = days * 86400 + hour * 3600 + minute * 60 + second;
  const auto offset_seconds = (sign == '-' ? -1 : 1) * zone * 15 * 60;
  unix_seconds = local_seconds - offset_seconds;
  return true;
}

bool sync_clock_from_modem() {
  auto result = spring::modem::execute("AT+QNTP=1,\"ntp.aliyun.com\",123", 70000);
  if (result != spring::modem::Result::ok)
    result = spring::modem::execute("AT+QNTP=1,\"time1.cloud.tencent.com\",123", 70000);
  if (result != spring::modem::Result::ok) return false;
  std::string response;
  if (!query_response("AT+CCLK?", response, 5000)) return false;
  std::int64_t unix_seconds{};
  if (!parse_cclk(response, unix_seconds)) return false;
  spring::clock::set_unix_seconds(unix_seconds);
  ESP_LOGI(kTag, "clock synchronized from China NTP source");
  return true;
}

bool configure_http() {
  return spring::modem::execute("AT+QHTTPCFG=\"contextid\",1", 5000) == spring::modem::Result::ok &&
         spring::modem::execute("AT+QHTTPCFG=\"sslctxid\",1", 5000) == spring::modem::Result::ok &&
         spring::modem::execute("AT+QSSLCFG=\"sslversion\",1,4", 5000) == spring::modem::Result::ok &&
         spring::modem::execute("AT+QSSLCFG=\"seclevel\",1,0", 5000) == spring::modem::Result::ok;
}

bool bring_up_cellular() {
  if (spring::modem::execute("AT", 3000) != spring::modem::Result::ok) return false;
  (void)spring::modem::execute("ATE0", 3000);
  (void)spring::modem::execute("AT+CPIN?", 5000);
  for (auto attempt = 0; attempt < 12; ++attempt) {
    std::string response;
    if (query_response("AT+CEREG?", response, 5000) &&
        (response.find(",1") != std::string::npos || response.find(",5") != std::string::npos)) break;
    vTaskDelay(pdMS_TO_TICKS(5000));
    if (attempt == 11) return false;
  }
  (void)spring::modem::execute("AT+CSQ", 5000);
  if (spring::modem::execute("AT+QIACT=1", 30000) != spring::modem::Result::ok) {
    if (spring::modem::execute("AT+QICSGP=1,1,\"CMNET\",\"\",\"\",1", 5000) != spring::modem::Result::ok ||
        spring::modem::execute("AT+QIACT=1", 30000) != spring::modem::Result::ok) return false;
  }
  if (!configure_http()) return false;
  cellular_available.store(true);
  return true;
}

void update_weather() {
#ifdef CONFIG_SPRING_OPENWEATHER_API_KEY
  const auto location = spring::modem::snapshot();
  if (!location.location_valid || CONFIG_SPRING_OPENWEATHER_API_KEY[0] == '\0') return;
  char url[384]{};
  std::snprintf(url, sizeof(url),
                "https://api.openweathermap.org/data/2.5/forecast?lat=%.6f&lon=%.6f&appid=%s&units=metric&lang=zh_cn",
                location.latitude, location.longitude, CONFIG_SPRING_OPENWEATHER_API_KEY);
  const auto response = spring::network::get(url);
  if (response.status != 200) return;
  auto* root = cJSON_Parse(response.body.c_str());
  if (root == nullptr) return;
  const auto* city = cJSON_GetObjectItemCaseSensitive(root, "city");
  const auto* timezone = city == nullptr ? nullptr : cJSON_GetObjectItemCaseSensitive(city, "timezone");
  const auto* entries = cJSON_GetObjectItemCaseSensitive(root, "list");
  spring::network::WeatherSnapshot next{};
  const auto timezone_seconds = cJSON_IsNumber(timezone) ? static_cast<std::int32_t>(timezone->valuedouble) : 0;
  if (cJSON_IsArray(entries)) {
    const auto total = cJSON_GetArraySize(entries);
    for (int index{}; index < total && next.count < next.forecast.size(); ++index) {
      const auto* entry = cJSON_GetArrayItem(entries, index);
      const auto* timestamp = entry == nullptr ? nullptr : cJSON_GetObjectItemCaseSensitive(entry, "dt");
      const auto* main = entry == nullptr ? nullptr : cJSON_GetObjectItemCaseSensitive(entry, "main");
      const auto* temperature = main == nullptr ? nullptr : cJSON_GetObjectItemCaseSensitive(main, "temp");
      const auto* conditions = entry == nullptr ? nullptr : cJSON_GetObjectItemCaseSensitive(entry, "weather");
      const auto* condition = cJSON_IsArray(conditions) ? cJSON_GetArrayItem(conditions, 0) : nullptr;
      const auto* description = condition == nullptr ? nullptr : cJSON_GetObjectItemCaseSensitive(condition, "description");
      if (!cJSON_IsNumber(timestamp) || !cJSON_IsNumber(temperature) || !cJSON_IsString(description)) continue;
      const auto utc_seconds = static_cast<std::time_t>(timestamp->valuedouble + timezone_seconds);
      std::tm local_time{};
      if (gmtime_r(&utc_seconds, &local_time) == nullptr) continue;
      auto& point = next.forecast[next.count++];
      point.hour = static_cast<std::uint8_t>(local_time.tm_hour);
      point.temperature_c = static_cast<std::int16_t>(std::lround(temperature->valuedouble));
      std::strncpy(point.description.data(), description->valuestring, point.description.size() - 1);
      point.description.back() = '\0';
    }
  }
  if (next.count > 0) {
    next.valid = true;
    if (weather_lock != nullptr && xSemaphoreTake(weather_lock, pdMS_TO_TICKS(100)) == pdTRUE) {
      next.revision = weather_state.revision + 1;
      weather_state = next;
      xSemaphoreGive(weather_lock);
    } else if (weather_lock == nullptr) {
      next.revision = weather_state.revision + 1;
      weather_state = next;
    }
  }
  cJSON_Delete(root);
#endif
}

void cellular_task(void*) {
  if (bring_up_cellular()) {
    (void)sync_clock_from_modem();
    (void)spring::location::refresh();
    update_weather();
    while (true) {
      vTaskDelay(pdMS_TO_TICKS(600'000));
      (void)spring::location::refresh();
      update_weather();
    }
  }
  ESP_LOGW(kTag, "EC600X network unavailable; retry on next boot");
  vTaskDelete(nullptr);
}
}

void spring::network::start() {
  weather_lock = xSemaphoreCreateMutex();
  install_fallback_weather();
  ESP_LOGI(kTag, "network service ready; EC600X is primary link");
  xTaskCreate(cellular_task, "cellular_net", 8192, nullptr, 4, nullptr);
}

void spring::network::set_link_available(Link link, bool available) {
  if (link == Link::wifi) wifi_available.store(available);
  if (link == Link::cellular) cellular_available.store(available);
}

spring::network::Link spring::network::active_link() {
  if (cellular_available.load()) return Link::cellular;
  if (wifi_available.load()) return Link::wifi;
  return Link::unavailable;
}

spring::network::Response spring::network::get(std::string_view url) {
  if (url.empty() || active_link() == Link::unavailable) return {.status = -1};
  if (active_link() == Link::cellular) {
    const auto response = spring::modem::http_get(url);
    return {.status = response.status, .body = response.body};
  }
  return {.status = -1};
}

spring::network::Response spring::network::post(std::string_view url, std::string_view body) {
  (void)url;
  (void)body;
  return {.status = -1};
}

spring::network::WeatherSnapshot spring::network::weather() {
  if (weather_lock != nullptr && xSemaphoreTake(weather_lock, pdMS_TO_TICKS(20)) == pdTRUE) {
    const auto snapshot = weather_state;
    xSemaphoreGive(weather_lock);
    return snapshot;
  }
  return weather_state;
}
