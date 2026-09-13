#include "declarative_ui.hpp"
#include "ui_core.hpp"
#include "ui_components.hpp"

namespace {
void render_xml_clock(spring::ui::Frame& frame, const spring::ui::Snapshot& snapshot) {
  frame.clear();
  frame.box({0, 0, 400, 300});
  spring::ui::components::draw_header(frame, snapshot);
  spring::ui::components::draw_countdown(frame);
  spring::ui::components::draw_date_panel(frame, snapshot);
  spring::ui::components::draw_dividers(frame);
  for (std::size_t index{}; index < 4; ++index) spring::ui::components::draw_forecast_card(frame, snapshot, index);
}
}

void spring::ui::render_declarative(Frame& target, const Snapshot& snapshot, const Router& router) {
  if (router.route() == Route::clock) {
    render_xml_clock(target, snapshot);
    return;
  }
  target.clear();
  target.box({0, 0, 400, 300});
  target.text_utf8(24, 24, Router::title(router.route()));
  if (router.route() == Route::network)
    target.text(120, 140, snapshot.registered ? "REGISTERED" : "SEARCHING");
  else if (router.route() == Route::location)
    target.text(120, 140, snapshot.locating ? "LOCATING" : "READY");
  else if (router.route() == Route::call)
    target.text(140, 140, snapshot.in_call ? "IN CALL" : "IDLE");
  else if (router.route() == Route::settings)
    target.text(110, 140, "SETTINGS");
  else
    target.text(150, 140, "SLEEP");
}
