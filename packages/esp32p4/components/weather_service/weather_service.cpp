#include "weather_service.hpp"

#include "cJSON.h"
#include "esp_log.h"
#include "location_service.hpp"
#include "network_service.hpp"
#include "sdkconfig.h"

#include <array>
#include <atomic>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <utility>

namespace {
constexpr char kTag[] = "weather";
spring::weather::Snapshot state{};
std::mutex state_mutex;
std::atomic_bool refreshing{};

void replace_snapshot(spring::weather::Snapshot next) {
  std::lock_guard lock(state_mutex);
  next.revision = state.revision + 1;
  state = std::move(next);
}

void install_fallback() {
  std::lock_guard lock(state_mutex);
  if (state.valid)
    return;
  spring::weather::Snapshot fallback{};
  fallback.valid = true;
  fallback.fake_data = true;
  fallback.count = static_cast<std::uint8_t>(fallback.forecast.size());
  constexpr std::array<std::uint8_t, spring::weather::kForecastSlots> hours{9, 12, 15, 18};
  constexpr std::array<std::int16_t, spring::weather::kForecastSlots> temperatures{22, 25, 24, 20};
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
  fallback.revision = state.revision + 1;
  state = std::move(fallback);
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
  const std::unique_ptr<cJSON, decltype(&cJSON_Delete)> document{
      cJSON_ParseWithLength(data == nullptr ? "" : data, response.body.size()), &cJSON_Delete};
  auto* root = document.get();
  if (root == nullptr) {
    ESP_LOGW(kTag, "weather response JSON parse failed");
    install_fallback();
    return;
  }

  const auto* city = cJSON_GetObjectItemCaseSensitive(root, "city");
  const auto* timezone =
      city == nullptr ? nullptr : cJSON_GetObjectItemCaseSensitive(city, "timezone");
  const auto* entries = cJSON_GetObjectItemCaseSensitive(root, "list");
  spring::weather::Snapshot next{};
  const auto timezone_seconds = cJSON_IsNumber(timezone) && std::isfinite(timezone->valuedouble) &&
                                        std::abs(timezone->valuedouble) <= 86'400
                                    ? static_cast<std::int32_t>(timezone->valuedouble)
                                    : 0;
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
      const auto* condition =
          cJSON_IsArray(conditions) ? cJSON_GetArrayItem(conditions, 0) : nullptr;
      const auto* description = condition == nullptr
                                    ? nullptr
                                    : cJSON_GetObjectItemCaseSensitive(condition, "description");
      if (!cJSON_IsNumber(timestamp) || !cJSON_IsNumber(temperature) ||
          !cJSON_IsString(description) || !std::isfinite(timestamp->valuedouble) ||
          timestamp->valuedouble < 0 || timestamp->valuedouble > 4'102'444'800.0 ||
          !std::isfinite(temperature->valuedouble) || std::abs(temperature->valuedouble) > 200) {
        continue;
      }
      const auto utc_seconds = static_cast<std::time_t>(timestamp->valuedouble + timezone_seconds);
      std::tm local_time{};
      if (gmtime_r(&utc_seconds, &local_time) == nullptr) {
        continue;
      }
      auto& point = next.forecast[next.count++];
      point.hour = static_cast<std::uint8_t>(local_time.tm_hour);
      point.temperature_c = static_cast<std::int16_t>(std::lround(temperature->valuedouble));
      point.temperature_low_c =
          cJSON_IsNumber(temperature_min) && std::isfinite(temperature_min->valuedouble) &&
                  std::abs(temperature_min->valuedouble) <= 200
              ? static_cast<std::int16_t>(std::lround(temperature_min->valuedouble))
              : point.temperature_c;
      point.temperature_high_c =
          cJSON_IsNumber(temperature_max) && std::isfinite(temperature_max->valuedouble) &&
                  std::abs(temperature_max->valuedouble) <= 200
              ? static_cast<std::int16_t>(std::lround(temperature_max->valuedouble))
              : point.temperature_c;
      std::strncpy(point.description.data(), description->valuestring,
                   point.description.size() - 1);
      point.description.back() = '\0';
    }
  }
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

void spring::weather::start() { install_fallback(); }

void spring::weather::refresh() {
  if (refreshing.exchange(true))
    return;
  struct RefreshGuard {
    bool submitted{};
    ~RefreshGuard() {
      if (!submitted)
        refreshing.store(false);
    }
  } guard;
#ifdef CONFIG_SPRING_OPENWEATHER_API_KEY
  if (CONFIG_SPRING_OPENWEATHER_API_KEY[0] == '\0') {
    ESP_LOGW(kTag, "weather refresh skipped: OpenWeather API key is empty");
    install_fallback();
    return;
  }
  if (!spring::location::current().location_valid)
    (void)spring::location::refresh();
  const auto location = spring::location::current();
  if (!location.location_valid) {
    ESP_LOGW(kTag, "weather refresh skipped: location is unavailable");
    install_fallback();
    return;
  }
  char url[384]{};
  std::snprintf(url, sizeof(url),
                "https://api.openweathermap.org/data/2.5/"
                "forecast?lat=%.6f&lon=%.6f&appid=%s&units=metric&lang=zh_cn",
                location.latitude, location.longitude, CONFIG_SPRING_OPENWEATHER_API_KEY);
  spring::network::Request request{};
  request.url = url;
  const auto id = spring::network::get(
      std::move(request), [](spring::network::RequestId, spring::network::Response response) {
        apply_response(std::move(response));
        refreshing.store(false);
      });
  guard.submitted = id != 0;
#else
  install_fallback();
#endif
}

spring::weather::Snapshot spring::weather::snapshot() {
  std::lock_guard lock(state_mutex);
  return state;
}
