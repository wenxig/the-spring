#pragma once

#include "ui_core.hpp"

namespace spring::ui::components {

void draw_header(Frame& frame, const Snapshot& snapshot);
void draw_countdown(Frame& frame);
void draw_date_panel(Frame& frame, const Snapshot& snapshot);
void draw_forecast_card(Frame& frame, const Snapshot& snapshot, std::size_t index);
void draw_dividers(Frame& frame);

}
