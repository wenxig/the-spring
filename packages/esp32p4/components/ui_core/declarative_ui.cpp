#include "declarative_ui.hpp"
#include "ui_core.hpp"

#ifdef ESP_PLATFORM
#include "lvgl.h"
#include <algorithm>
#include <array>
#include <cstdio>

namespace {
lv_display_t* display = nullptr;
lv_obj_t* time_label = nullptr;
lv_obj_t* date_label = nullptr;
std::array<lv_obj_t*, 7> weekday_labels{};
std::array<lv_obj_t*, 4> forecast_labels{};
std::array<lv_obj_t*, 4> forecast_hours{};
spring::ui::Frame* target_frame = nullptr;
std::array<std::uint8_t, spring::ui::kBytes + 8> draw_buffer{};

void flush(lv_display_t* disp, const lv_area_t* area, std::uint8_t* pixels) {
  (void)disp;
  if (target_frame == nullptr || area == nullptr || pixels == nullptr) return;
  const auto width = static_cast<std::uint16_t>(lv_area_get_width(area));
  const auto height = static_cast<std::uint16_t>(lv_area_get_height(area));
  constexpr std::size_t palette_bytes = 8;
  const auto* source = pixels + palette_bytes;
  const auto stride = static_cast<std::size_t>((width + 7U) / 8U);
  for (std::uint16_t row{}; row < height; ++row) {
    for (std::uint16_t column{}; column < width; ++column) {
      const auto bit = static_cast<std::uint8_t>(0x80U >> (column % 8U));
      // LVGL I1 uses a set bit for the light palette entry; Frame stores set bits as black.
      const auto black = (source[static_cast<std::size_t>(row) * stride + column / 8U] & bit) == 0;
      target_frame->pixel(static_cast<std::uint16_t>(area->x1 + column),
                          static_cast<std::uint16_t>(area->y1 + row), black);
    }
  }
  lv_display_flush_ready(disp);
}

void ensure_ui() {
  if (display != nullptr) return;
  lv_init();
  display = lv_display_create(spring::ui::kWidth, spring::ui::kHeight);
  lv_display_set_color_format(display, LV_COLOR_FORMAT_I1);
  lv_display_set_render_mode(display, LV_DISPLAY_RENDER_MODE_FULL);
  lv_display_set_buffers(display, draw_buffer.data(), nullptr, draw_buffer.size(), LV_DISPLAY_RENDER_MODE_FULL);
  lv_display_set_flush_cb(display, flush);
  auto* screen = lv_obj_create(nullptr);
  lv_obj_set_style_bg_color(screen, lv_color_black(), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, LV_PART_MAIN);
  time_label = lv_label_create(screen);
  lv_obj_set_pos(time_label, 74, 45);
  lv_obj_set_style_text_color(time_label, lv_color_white(), LV_PART_MAIN);
  lv_obj_set_style_text_font(time_label, &lv_font_montserrat_14, LV_PART_MAIN);
  date_label = lv_label_create(screen);
  lv_obj_set_pos(date_label, 29, 226);
  lv_obj_set_style_text_color(date_label, lv_color_white(), LV_PART_MAIN);
  constexpr const char* weekdays[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
  for (std::size_t index{}; index < weekday_labels.size(); ++index) {
    weekday_labels[index] = lv_label_create(screen);
    lv_obj_set_pos(weekday_labels[index], 323, static_cast<lv_coord_t>(16 + index * 20));
    lv_label_set_text(weekday_labels[index], weekdays[index]);
  }
  for (std::size_t index{}; index < forecast_labels.size(); ++index) {
    const auto x = static_cast<lv_coord_t>(106 + index * 70);
    forecast_labels[index] = lv_label_create(screen);
    forecast_hours[index] = lv_label_create(screen);
    lv_obj_set_pos(forecast_labels[index], x, 250);
    lv_obj_set_pos(forecast_hours[index], x, 276);
  }
  lv_screen_load(screen);
}
}

void spring::ui::render_declarative(Frame& target, const Snapshot& snapshot) {
  ensure_ui();
  target.clear();
  target_frame = &target;
  char time[6]{};
  char date[16]{};
  std::snprintf(time, sizeof(time), "%02u:%02u", static_cast<unsigned>(snapshot.hour % 24U),
                static_cast<unsigned>(snapshot.minute % 60U));
  std::snprintf(date, sizeof(date), "%u/%u", static_cast<unsigned>(snapshot.month),
                static_cast<unsigned>(snapshot.day));
  lv_label_set_text(time_label, time);
  lv_label_set_text(date_label, date);
  constexpr const char* weekdays[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
  for (std::size_t index{}; index < weekday_labels.size(); ++index)
    lv_label_set_text(weekday_labels[index], weekdays[index]);
  for (std::size_t index{}; index < forecast_labels.size(); ++index) {
    char value[8]{}, hour[8]{};
    if (index < snapshot.forecast_count && index < snapshot.forecast.size()) {
      std::snprintf(value, sizeof(value), "%dC", snapshot.forecast[index].temperature_c);
      std::snprintf(hour, sizeof(hour), "%02u:00", snapshot.forecast[index].hour);
    } else {
      std::snprintf(value, sizeof(value), "--");
      std::snprintf(hour, sizeof(hour), "--:--");
    }
    lv_label_set_text(forecast_labels[index], value);
    lv_label_set_text(forecast_hours[index], hour);
  }
  lv_obj_invalidate(lv_screen_active());
  lv_timer_handler();
  target_frame = nullptr;
}
#else
void spring::ui::render_declarative(Frame& target, const Snapshot& snapshot) {
  Router router;
  router.render(target, snapshot);
}
#endif
