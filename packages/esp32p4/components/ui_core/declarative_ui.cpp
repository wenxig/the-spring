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
      const auto black = (source[static_cast<std::size_t>(row) * stride + column / 8U] & bit) != 0;
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
