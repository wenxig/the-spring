#include "weather_service.hpp"

#include "cJSON.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "location_service.hpp"
#include "network_service.hpp"
#include "sdkconfig.h"

#include <array>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <string>
#include <string_view>
#include <utility>

namespace {
constexpr char kTag[] = "weather";
spring::weather::Snapshot state{};
SemaphoreHandle_t state_lock{nullptr};

void replace_snapshot(spring::weather::Snapshot next) {
  if (state_lock != nullptr && xSemaphoreTake(state_lock, pdMS_TO_TICKS(100)) != pdTRUE) {
    return;
  }
  next.revision = state.revision + 1;
  state = std::move(next);
  if (state_lock != nullptr) {
    xSemaphoreGive(state_lock);
  }
}

void install_fallback() {
  spring::weather::Snapshot fallback{};
  fallback.valid = true;
  fallback.fake_data = true;
  fallback.count = static_cast<std::uint8_t>(fallback.forecast.size());
  constexpr std::array<std::uint8_t, spring::weather::kForecastSlots> hours{9, 12, 15, 18};
  constexpr std::array<std::int16_t, spring::weather::kForecastSlots> temperatures{22, 25, 24,
                                                                                    20};
  constexpr std::array<std::int16_t, spring::weather::kForecastSlots> lows{20, 22, 22, 18};
  constexpr std::array<std::int16_t, spring::weather::kForecastSlots> highs{24, 27, 26, 22};
  for (std::size_t index{}; index < fallback.forecast.size(); ++index) {
    auto& point = fallback.forecast[index];
    point.hour = hours[index];
    point.temperature_c = temperatures[index];
    point.temperature_low_c = lows[index];
    point.temperature_high_c = highs[index];
    std::strncpy(point.description.data(), "晴", point.description.size() - 1);
    point.description.back() = '\0';
  }
  replace_snapshot(std::move(fallback));
}

void apply_response(spring::network::Response response) {
  if (!response.ok()) {
    ESP_LOGW(kTag, "weather request failed: link=%d status=%d error=%d detail=%s",
             static_cast<int>(response.link), response.status, static_cast<int>(response.error),
             response.error_detail.c_str());
    install_fallback();
    return;
  }
  const auto* data = reinterpret_cast<const char*>(response.body.data());
  auto* root = cJSON_ParseWithLength(data == nullptr ? "" : data, response.body.size());
  if (root == nullptr) {
    ESP_LOGW(kTag, "weather response JSON parse failed");
    install_fallback();
    return;
  }

  const auto* city = cJSON_GetObjectItemCaseSensitive(root, "city");
  const auto* timezone = city == nullptr ? nullptr : cJSON_GetObjectItemCaseSensitive(city, "timezone");
  const auto* entries = cJSON_GetObjectItemCaseSensitive(root, "list");
  spring::weather::Snapshot next{};
  const auto timezone_seconds =
      cJSON_IsNumber(timezone) ? static_cast<std::int32_t>(timezone->valuedouble) : 0;
  if (cJSON_IsArray(entries)) {
    const auto total = cJSON_GetArraySize(entries);
    for (int index{}; index < total && next.count < next.forecast.size(); ++index) {
      const auto* entry = cJSON_GetArrayItem(entries, index);
      const auto* timestamp =
          entry == nullptr ? nullptr : cJSON_GetObjectItemCaseSensitive(entry, "dt");
      const auto* main =
          entry == nullptr ? nullptr : cJSON_GetObjectItemCaseSensitive(entry, "main");
      const auto* temperature =
          main == nullptr ? nullptr : cJSON_GetObjectItemCaseSensitive(main, "temp");
      const auto* temperature_min =
          main == nullptr ? nullptr : cJSON_GetObjectItemCaseSensitive(main, "temp_min");
      const auto* temperature_max =
          main == nullptr ? nullptr : cJSON_GetObjectItemCaseSensitive(main, "temp_max");
      const auto* conditions =
          entry == nullptr ? nullptr : cJSON_GetObjectItemCaseSensitive(entry, "weather");
      const auto* condition = cJSON_IsArray(conditions) ? cJSON_GetArrayItem(conditions, 0) : nullptr;
      const auto* description = condition == nullptr
                                    ? nullptr
                                    : cJSON_GetObjectItemCaseSensitive(condition, "description");
      if (!cJSON_IsNumber(timestamp) || !cJSON_IsNumber(temperature) ||
          !cJSON_IsString(description)) {
        continue;
      }
      const auto utc_seconds =
          static_cast<std::time_t>(timestamp->valuedouble + timezone_seconds);
      std::tm local_time{};
      if (gmtime_r(&utc_seconds, &local_time) == nullptr) {
        continue;
      }
      auto& point = next.forecast[next.count++];
      point.hour = static_cast<std::uint8_t>(local_time.tm_hour);
      point.temperature_c = static_cast<std::int16_t>(std::lround(temperature->valuedouble));
      point.temperature_low_c = cJSON_IsNumber(temperature_min)
                                    ? static_cast<std::int16_t>(std::lround(temperature_min->valuedouble))
                                    : point.temperature_c;
      point.temperature_high_c = cJSON_IsNumber(temperature_max)
                                     ? static_cast<std::int16_t>(std::lround(temperature_max->valuedouble))
                                     : point.temperature_c;
      std::strncpy(point.description.data(), description->valuestring,
                    point.description.size() - 1);
      point.description.back() = '\0';
    }
  }
  cJSON_Delete(root);
  if (next.count == 0) {
    ESP_LOGW(kTag, "weather response contained no usable forecast entries");
    install_fallback();
    return;
  }
  next.valid = true;
  const auto forecast_count = next.count;
  replace_snapshot(std::move(next));
  ESP_LOGI(kTag, "weather updated: forecast_count=%u link=%d", forecast_count,
           static_cast<int>(response.link));
}

} // namespace

void spring::weather::start() {
  if (state_lock == nullptr) {
    state_lock = xSemaphoreCreateMutex();
  }
  install_fallback();
}

void spring::weather::refresh() {
#ifdef CONFIG_SPRING_OPENWEATHER_API_KEY
  if (CONFIG_SPRING_OPENWEATHER_API_KEY[0] == '\0') {
    ESP_LOGW(kTag, "weather refresh skipped: OpenWeather API key is empty");
    install_fallback();
    return;
  }
  const auto location = spring::location::current();
  if (!location.location_valid) {
    ESP_LOGW(kTag, "weather refresh skipped: location is unavailable");
    install_fallback();
    return;
  }
  char url[384]{};
  std::snprintf(url, sizeof(url),
                "https://api.openweathermap.org/data/2.5/forecast?lat=%.6f&lon=%.6f&appid=%s&units=metric&lang=zh_cn",
                location.latitude, location.longitude, CONFIG_SPRING_OPENWEATHER_API_KEY);
  spring::network::Request request{};
  request.url = url;
  (void)spring::network::get(std::move(request), [](spring::network::RequestId,
                                                   spring::network::Response response) {
    apply_response(std::move(response));
  });
#else
  install_fallback();
#endif
}

spring::weather::Snapshot spring::weather::snapshot() {
  if (state_lock != nullptr && xSemaphoreTake(state_lock, pdMS_TO_TICKS(20)) == pdTRUE) {
    const auto copy = state;
    xSemaphoreGive(state_lock);
    return copy;
  }
  return state;
}
