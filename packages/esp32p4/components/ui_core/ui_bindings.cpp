#include "declarative_ui.hpp"
#include "lvgl.h"
#if LV_TINY_TTF_FILE_SUPPORT == 0
#include "font_asset.hpp"
#include "ionicons_assets.hpp"
#include "src/misc/cache/instance/lv_image_cache.h"
#endif
#include "ui_core.hpp"

#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <span>
#include <string_view>
#include <utility>

namespace spring::ui {
namespace {
enum class Kind : std::uint8_t { label, rule, icon };
struct Element {
  Kind kind;
  int x, y, width, height, size, slot;
  bool centered;
  const char* text;
  std::string_view binding;
};
#include "ui_scene.inc"

const ForecastPoint* forecast(const Snapshot& snapshot, int slot) {
  if (!snapshot.weather_valid || slot < 0 ||
      static_cast<std::size_t>(slot) >= snapshot.forecast.size() ||
      std::cmp_greater_equal(slot, snapshot.forecast_count))
    return nullptr;
  return &snapshot.forecast[static_cast<std::size_t>(slot)];
}

const char* weather_icon(const ForecastPoint* point) {
#if LV_TINY_TTF_FILE_SUPPORT != 0
  if (point == nullptr)
    return "S:/icons/help-circle-outline.svg";
  const auto text = std::string_view{point->description.data()};
  const auto contains = [&](std::string_view value) {
    return text.find(value) != std::string_view::npos;
  };
  if (contains("雷") || contains("storm"))
    return "S:/icons/thunderstorm-outline.svg";
  if (contains("雪") || contains("snow"))
    return "S:/icons/snow-outline.svg";
  if (contains("雨") || contains("rain"))
    return "S:/icons/rainy-outline.svg";
  if (contains("阴") || contains("overcast"))
    return "S:/icons/cloudy-outline.svg";
  const auto night = point->hour < 6 || point->hour >= 18;
  if (contains("云") || contains("cloud"))
    return night ? "S:/icons/cloudy-night-outline.svg" : "S:/icons/partly-sunny-outline.svg";
  if (contains("晴") || contains("clear"))
    return night ? "S:/icons/moon-outline.svg" : "S:/icons/sunny-outline.svg";
  return "S:/icons/help-circle-outline.svg";
#else
  if (point == nullptr)
    return ion5_help_circle_outline;
  const auto text = std::string_view{point->description.data()};
  const auto contains = [&](std::string_view value) {
    return text.find(value) != std::string_view::npos;
  };
  if (contains("雷") || contains("storm"))
    return ion5_thunderstorm_outline;
  if (contains("雪") || contains("snow"))
    return ion5_snow_outline;
  if (contains("雨") || contains("rain"))
    return ion5_rainy_outline;
  if (contains("阴") || contains("overcast"))
    return ion5_cloudy_outline;
  const auto night = point->hour < 6 || point->hour >= 18;
  if (contains("云") || contains("cloud"))
    return night ? ion5_cloudy_night_outline : ion5_partly_sunny_outline;
  if (contains("晴") || contains("clear"))
    return night ? ion5_moon_outline : ion5_sunny_outline;
  return ion5_help_circle_outline;
#endif
}

const char* bound_text(const Element& element, const Snapshot& snapshot, const Router& router,
                       std::span<char> buffer) {
  const auto binding = element.binding;
  const auto number = [&](const char* format, auto... values) {
    std::snprintf(buffer.data(), buffer.size(), format, values...);
    return buffer.data();
  };
  if (binding.empty())
    return element.text;
  if (binding == "snapshot.location")
    return snapshot.location == nullptr || snapshot.location[0] == '\0' ? "位置待更新"
                                                                        : snapshot.location;
  if (binding == "route.title")
    return Router::title(router.route());
  if (binding == "route.state") {
    switch (router.route()) {
    case Route::network:
      return snapshot.registered ? "网络已注册" : "正在搜索网络";
    case Route::location:
      return snapshot.locating ? "正在定位" : "定位就绪";
    case Route::call:
      return snapshot.in_call ? "通话中" : "待机";
    case Route::settings:
      return number("当前选项 %u\n亮度 / 自动休眠 / 版本", router.setting_index() + 1U);
    case Route::sleep:
      return "休眠";
    case Route::clock:
      return "";
    }
  }
  if (binding.starts_with("snapshot.")) {
    if (!snapshot.date_valid)
      return binding == "snapshot.time"        ? "--:--"
             : binding == "snapshot.countdown" ? "等待校时"
                                               : "--";
    if (binding == "snapshot.period")
      return snapshot.hour < 12 ? "上午" : "下午";
    if (binding == "snapshot.time")
      return number("%02u:%02u", snapshot.hour, snapshot.minute);
    if (binding == "snapshot.countdown") {
      if (snapshot.exam_active)
        return "高考进行中 6/7—6/9";
      return number("距离高考还有 %u 天", snapshot.countdown_days);
    }
    if (binding == "snapshot.year")
      return number("%u", snapshot.year);
    if (binding == "snapshot.date")
      return number("%u/%u", snapshot.month, snapshot.day);
    if (binding == "snapshot.month")
      return number("%u", snapshot.month);
    if (binding == "snapshot.day")
      return number("%u", snapshot.day);
    if (binding == "snapshot.weekday") {
      constexpr std::array weekdays{"星期日", "星期一", "星期二", "星期三",
                                    "星期四", "星期五", "星期六"};
      return weekdays[snapshot.weekday % weekdays.size()];
    }
  }
  const auto* point = forecast(snapshot, element.slot);
  if (binding == "forecast.label") {
    if (element.slot == 3)
      return "明天";
    return point == nullptr ? "--:--" : number("%02u:00", point->hour);
  }
  if (point == nullptr)
    return "--";
  if (binding == "forecast.high")
    return number("%d°", point->temperature_high_c);
  if (binding == "forecast.low")
    return number("%d°", point->temperature_low_c);
  if (binding == "forecast.description")
    return point->description.data();
  return "";
}

class Renderer {
public:
  Renderer() {
    lv_init();
    display_ = lv_display_create(kWidth, kHeight);
    LV_ASSERT_NULL(display_);
    lv_display_set_color_format(display_, LV_COLOR_FORMAT_RGB565);
    lv_display_set_user_data(display_, this);
    lv_display_set_flush_cb(display_, flush);
    buffer_ = lv_draw_buf_create(kWidth, kHeight, LV_COLOR_FORMAT_RGB565, 0);
    LV_ASSERT_NULL(buffer_);
    lv_display_set_draw_buffers(display_, buffer_, nullptr);
    lv_display_set_render_mode(display_, LV_DISPLAY_RENDER_MODE_FULL);
    lv_display_set_theme(display_, nullptr);
  }
  ~Renderer() {
    lv_display_delete(display_);
    for (std::size_t index{}; index < fonts_.size(); ++index)
      if (fonts_[index] != nullptr && font_owned_[index])
        lv_tiny_ttf_destroy(fonts_[index]);
    lv_draw_buf_destroy(buffer_);
  }
  Renderer(const Renderer&) = delete;
  Renderer& operator=(const Renderer&) = delete;
  Renderer(Renderer&&) = delete;
  Renderer& operator=(Renderer&&) = delete;

  void render(Frame& target, const Snapshot& snapshot, const Router& router) {
    auto* root = lv_display_get_screen_active(display_);
    lv_obj_clean(root);
    lv_obj_remove_style_all(root);
    lv_obj_set_style_bg_color(root, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(root, LV_OPA_COVER, 0);
    lv_obj_set_style_text_font(root, font_for(16), 0);
    lv_obj_remove_flag(root, LV_OBJ_FLAG_SCROLLABLE);
    const auto scene = router.route() == Route::clock ? std::span<const Element>{clock_scene}
                                                      : std::span<const Element>{status_scene};
#if LV_TINY_TTF_FILE_SUPPORT == 0
    auto icon_index = std::size_t{};
#endif
    for (const auto& element : scene) {
      if (element.kind == Kind::icon) {
#if LV_TINY_TTF_FILE_SUPPORT != 0
        auto* object = lv_image_create(root);
        lv_image_set_src(object, element.binding == "location.icon"
                                     ? "S:/icons/location-outline.svg"
                                     : weather_icon(forecast(snapshot, element.slot)));
#else
        auto& descriptor = icons_[icon_index++];
        lv_image_cache_drop(&descriptor);
        const auto* svg = element.binding == "location.icon"
                              ? ion5_location_outline
                              : weather_icon(forecast(snapshot, element.slot));
        descriptor = {};
        descriptor.header.magic = LV_IMAGE_HEADER_MAGIC;
        descriptor.header.w = 512;
        descriptor.header.h = 512;
        descriptor.header.cf = LV_COLOR_FORMAT_RAW;
        descriptor.data_size = static_cast<std::uint32_t>(std::strlen(svg));
        descriptor.data = reinterpret_cast<const std::uint8_t*>(svg);
        auto* object = lv_image_create(root);
        lv_image_set_src(object, &descriptor);
#endif
        lv_obj_set_size(object, element.width, element.height);
        lv_image_set_inner_align(object, LV_IMAGE_ALIGN_CONTAIN);
        lv_obj_set_pos(object, element.x, element.y);
      } else if (element.kind == Kind::label) {
        auto* object = lv_label_create(root);
        std::array<char, 128> text{};
        const auto* value = bound_text(element, snapshot, router, text);
        lv_label_set_text(object, value);
        lv_label_set_long_mode(object, LV_LABEL_LONG_CLIP);
        lv_obj_set_pos(object, element.x, element.y);
        lv_obj_set_size(object, element.width, element.height);
        lv_obj_set_style_text_color(object, lv_color_black(), 0);
        auto size = element.size;
        if (element.binding == "snapshot.location") {
          auto extent = lv_point_t{};
          for (;;) {
            lv_text_get_size(&extent, value, font_for(size), 0, 0, LV_COORD_MAX, LV_TEXT_FLAG_NONE);
            if (extent.x <= element.width || size <= 10)
              break;
            --size;
          }
          lv_label_set_long_mode(object, LV_LABEL_LONG_DOT);
        }
        lv_obj_set_style_text_font(object, font_for(size), 0);
        lv_obj_set_style_text_align(
            object, element.centered ? LV_TEXT_ALIGN_CENTER : LV_TEXT_ALIGN_LEFT, 0);
      } else {
        auto* object = lv_obj_create(root);
        lv_obj_remove_style_all(object);
        lv_obj_set_pos(object, element.x, element.y);
        lv_obj_set_size(object, element.width, element.height);
        lv_obj_set_style_bg_color(object, lv_color_black(), 0);
        lv_obj_set_style_bg_opa(object, LV_OPA_COVER, 0);
      }
    }
    target.clear();
    active_frame_ = &target;
    lv_obj_invalidate(root);
    lv_refr_now(display_);
    active_frame_ = nullptr;
  }

private:
  lv_font_t* font_for(int size) {
    auto& font = fonts_[static_cast<std::size_t>(size)];
#if LV_TINY_TTF_FILE_SUPPORT != 0
    if (font == nullptr) {
      font = lv_tiny_ttf_create_file("S:/fonts/HYWenHei-65W-3.ttf", size);
      if (font != nullptr)
        font_owned_[static_cast<std::size_t>(size)] = true;
    }
    if (font == nullptr)
      font = const_cast<lv_font_t*>(&lv_font_montserrat_14);
#else
    if (font == nullptr) {
      font = lv_tiny_ttf_create_data(default_font_data, default_font_size, size);
      font_owned_[static_cast<std::size_t>(size)] = true;
    }
#endif
    LV_ASSERT_NULL(font);
    return font;
  }
  static void flush(lv_display_t* display, const lv_area_t* area, std::uint8_t* pixels) {
    auto& self = *static_cast<Renderer*>(lv_display_get_user_data(display));
    if (self.active_frame_ != nullptr) {
      const auto width = lv_area_get_width(area);
      const auto stride =
          lv_draw_buf_width_to_stride(static_cast<std::uint32_t>(width), LV_COLOR_FORMAT_RGB565);
      for (auto row = 0; row < lv_area_get_height(area); ++row) {
        for (auto column = 0; column < width; ++column) {
          std::uint16_t color{};
          std::memcpy(&color,
                      pixels + static_cast<std::size_t>(row) * stride +
                          static_cast<std::size_t>(column) * 2,
                      2);
          const auto red = ((color >> 11U) & 31U) * 255U / 31U;
          const auto green = ((color >> 5U) & 63U) * 255U / 63U;
          const auto blue = (color & 31U) * 255U / 31U;
          self.active_frame_->pixel(static_cast<std::uint16_t>(area->x1 + column),
                                    static_cast<std::uint16_t>(area->y1 + row),
                                    red * 30U + green * 59U + blue * 11U < 12800U);
        }
      }
    }
    lv_display_flush_ready(display);
  }
  lv_display_t* display_{};
  lv_draw_buf_t* buffer_{};
  Frame* active_frame_{};
  std::array<lv_font_t*, 97> fonts_{};
  std::array<bool, 97> font_owned_{};
#if LV_TINY_TTF_FILE_SUPPORT == 0
  std::array<lv_image_dsc_t, std::size(clock_scene)> icons_{};
#endif
};
} // namespace

void render_lvgl_declarative(Frame& target, const Snapshot& snapshot, const Router& router) {
  // The application render mutex serializes all LVGL access.
  static Renderer renderer;
  renderer.render(target, snapshot, router);
}
} // namespace spring::ui
