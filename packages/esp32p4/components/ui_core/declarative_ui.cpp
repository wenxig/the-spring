#include "declarative_ui.hpp"
#include "ui_core.hpp"
void spring::ui::render_declarative(Frame& target, const Snapshot& snapshot, const Router& router) {
  render_lvgl_declarative(target, snapshot, router);
}
