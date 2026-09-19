#pragma once

namespace spring::ui {
class Frame;
struct Snapshot;
class Router;

/** Render the XML-defined screen selected by the global router. */
void render_declarative(Frame& target, const Snapshot& snapshot, const Router& router);
void render_lvgl_declarative(Frame& target, const Snapshot& snapshot, const Router& router);
}
