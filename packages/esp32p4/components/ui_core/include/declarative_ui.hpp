#pragma once

namespace spring::ui {
class Frame;
struct Snapshot;

/** Render the XML-defined screen through the LVGL 9.5 software renderer. */
void render_declarative(Frame& target, const Snapshot& snapshot);
}
